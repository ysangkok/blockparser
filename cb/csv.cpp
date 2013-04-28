
// Full CSV dump of the blockchain

#include <util.h>
#include <stdio.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <callback.h>

typedef GoogMap<Hash256, uint64_t, Hash256Hasher, Hash256Equal>::Map OutputMap;

struct CSVDump:public Callback
{
    FILE *txFile;
    FILE *blockFile;
    FILE *inputFile;
    FILE *outputFile;

    optparse::OptionParser parser;
    int64_t firstBlock;
    int64_t lastBlock;

    uint32_t active;

    // Counters used across functions
    uint64_t blkID;
    uint64_t blkSize;
    uint64_t totalBlkFees;
    uint64_t totalBlkOutput;
    uint64_t numBlkTxs;

    uint64_t txID;
    uint32_t genTx;
    const uint8_t *txStart;
    uint64_t numTxInputs;
    uint64_t numTxOutputs;
    uint64_t totalTxInput;
    uint64_t totalTxOutput;

    CSVDump()
    {
        parser
            .usage("[options]")
            .version("")
            .description("create an CSV dump of the blockchain")
            .epilog("")
        ;
        parser
            .add_option("-f", "--firstBlock")
            .action("store")
            .type("int")
            .set_default(0)
            .help("first block to dump (default: 0)")
        ;
        parser
            .add_option("-l", "--lastBlock")
            .action("store")
            .type("int")
            .set_default(-1)
            .help("last block to dump (default: last block)")
        ;
    }

    virtual const char                   *name() const         { return "csvdump"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                         needTXHash() const    { return true;      }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("csv");
    }

    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        txID = 0;
        blkID = 0;
        active = 0;

        optparse::Values &values = parser.parse_args(argc, argv);
        firstBlock = values.get("firstBlock");
        lastBlock = values.get("lastBlock");

        info("Dumping the blockchain...");

        blockFile = fopen("blocks.csv", "w");
        if(!blockFile) sysErrFatal("couldn't open file blocks.csv for writing\n");
        fprintf(blockFile, "ID,Hash,Version,Timestamp,Nonce,Difficulty,Merkle,NumTransactions,OutputValue,FeesValue,Size\n");

        txFile = fopen("transactions.csv", "w");
        if(!txFile) sysErrFatal("couldn't open file txs.csv for writing\n");
        fprintf(txFile, "ID,Hash,Version,BlockId,NumInputs,NumOutputs,OutputValue,FeesValue,LockTime,Size\n");

        outputFile = fopen("outputs.csv", "w");
        if(!outputFile) sysErrFatal("couldn't open file outputs.csv for writing\n");
        fprintf(outputFile, "TransactionId,Index,Value,Script,ReceivingAddress,InputTxHash,InputTxIndex\n");

        inputFile = fopen("inputs.csv", "w");
        if(!inputFile) sysErrFatal("couldn't open file inputs.csv for writing\n");
        fprintf(inputFile, "TransactionId,Index,Script,OutputTxHash,OutputTxIndex\n");

        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t chainBase
    )
    {
        if (lastBlock >= 0 && lastBlock < b->height - 1) wrapup();
        if (b->height - 1 == firstBlock) active = 1;

        if (active) {
            uint8_t blockHash[kSHA256ByteSize];
            sha256Twice(blockHash, b->data, 80);

            const uint8_t *p = b->data;

            // Total block size is 81 bytes plus transactions
            blkSize = 81;

            totalBlkFees = 0;
            totalBlkOutput = 0;
            numBlkTxs = 0;

            LOAD(uint32_t, version, p);
            SKIP(uint256_t, prevBlkHash, p);
            LOAD(uint256_t, blkMerkleRoot, p);
            LOAD(uint32_t, blkTime, p);
            LOAD(uint32_t, difficultyBits, p);
            LOAD(uint32_t, nonce, p);

            // ID
            fprintf(blockFile, "%" PRIu64 ",", blkID);

            // Hash
            uint8_t buf[1 + 2*kSHA256ByteSize];
            toHex(buf, blockHash);
            fprintf(blockFile, "\"%s\",", buf);

            // Version
            fprintf(blockFile, "%" PRIu32 ",", version);

            // Timestamp
            time_t blockTime = blkTime;
            char tbuf[23];
            strftime(tbuf, sizeof tbuf, "%FT%TZ", gmtime(&blockTime));

            fprintf(blockFile, "\"%s\",", tbuf);

            // Nonce
            fprintf(blockFile, "%" PRIu32 ",", nonce);

            // Difficulty
            fprintf(blockFile, "%f,", difficulty(difficultyBits));

            // Merkle root
            uint8_t buf2[kSHA256ByteSize];
            uint8_t buf3[1 + 2*kSHA256ByteSize];
            memcpy(buf2, &blkMerkleRoot, kSHA256ByteSize);
            toHex(buf3, buf2);
            fprintf(blockFile, "\"%s\",", buf3);
        }
    }

    virtual void endBlock(
        const Block *b
    )
    {
        if (active)
        {
            // Number of transactions
            fprintf(blockFile, "%" PRIu64 ",", numBlkTxs);

            // Value of output transactions
            fprintf(blockFile, "%" PRIu64 ",", totalBlkOutput);

            // Value of fees
            fprintf(blockFile, "%" PRIu64 ",", totalBlkFees);

            // Size; depends on number of transactions
            if (numBlkTxs < 253)
            {
                // Nothing to do
            }
            else if (numBlkTxs < 65536)
            {
                blkSize += 2;
            }
            else if (numBlkTxs < 4294967296L)
            {
                blkSize += 4;
            }
            else
            {
                blkSize += 8;
            }
            fprintf(blockFile, "%" PRIu64 "\n", blkSize);
        }

        blkID++;
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    )
    {
        if (active)
        {
            txStart = p;
            numBlkTxs++;
            numTxInputs = 0;
            numTxOutputs = 0;
            totalTxInput = 0;
            totalTxOutput = 0;

            // ID
            fprintf(txFile, "%" PRIu64 ",", txID);

            // Hash
            uint8_t buf[1 + 2*kSHA256ByteSize];
            toHex(buf, hash);
            fprintf(txFile, "\"%s\",", buf);

            // Version
            LOAD(uint32_t, version, p);
            fprintf(txFile, "%" PRIu32 ",", version);

            // Block ID
            fprintf(txFile, "%" PRIu64 ",", blkID);
        }
    }

    virtual void endTX(
        const uint8_t *p
    )
    {
        if (active)
        {
            // Number of inputs
            fprintf(txFile, "%" PRIu64 ",", numTxInputs);

            // Number of outputs
            fprintf(txFile, "%" PRIu64 ",", numTxOutputs);

            // Value of outputs
            fprintf(txFile, "%" PRIu64 ",", totalTxOutput);

            // Value of fees
            if (genTx)
            {
                fprintf(txFile, "0,");
            }
            else
            {
                fprintf(txFile, "%" PRIu64 ",", totalTxInput - totalTxOutput);
                totalBlkFees += totalTxInput - totalTxOutput;
            }

            // Lock time
            LOAD(uint32_t, lockTime, p);
            fprintf(txFile, "%" PRIu32 ",", lockTime);

            // Size
            // p is 4 bigger than it should be due to above lock time load
            uint64_t txSize = p - 4 - txStart;
            blkSize += txSize;
            fprintf(txFile, "%" PRIu64 "\n", txSize);

            totalBlkOutput += totalTxOutput;
        }
        txID++;
    }

    virtual void endOutput(
        const uint8_t *p,
        uint64_t      value,
        const uint8_t *txHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize
    )
    {
        if (active)
        {
            numTxOutputs++;
            totalTxOutput += value;

            // Script
            uint8_t script[1 + 2*outputScriptSize];
            toHex(script, outputScript, outputScriptSize);

            // Receiving address
            uint8_t address[40];
            address[0] = 'X';
            address[1] = 0;
            uint8_t addrType[3];
            uint160_t pubKeyHash;
            int type = solveOutputScript(pubKeyHash.v, outputScript, outputScriptSize, addrType);
            if(likely(0<=type)) hash160ToAddr(address, pubKeyHash.v);

            // N.B. Input hash and index are NULL at this stage
            fprintf(outputFile, "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",\"%s\",\"%s\",,\n", txID, outputIndex, value, script, address);
        }
    }

    virtual void startInput(
        const uint8_t *p
    )
    {
        if (active)
        {
            static uint256_t gNullHash;
            LOAD(uint256_t, upTXHash, p);
            if (0==memcmp(gNullHash.v, upTXHash.v, sizeof(gNullHash))) {
                genTx = 1;
                uint64_t reward = getBaseReward(blkID);
                totalTxInput += reward;
            }
            else
            {
                genTx = 0;
            }
        }
    }


    virtual void edge(
        uint64_t      value,
        const uint8_t *upTXHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize,
        const uint8_t *downTXHash,
        uint64_t      inputIndex,
        const uint8_t *inputScript,
        uint64_t      inputScriptSize
    )
    {
        if (active)
        {
            numTxInputs++;
            totalTxInput += value;

            // outputTxHash
            uint8_t outputTxHash[1 + 2*kSHA256ByteSize];
            toHex(outputTxHash, upTXHash);

            // Script
            uint8_t script[1 + 2*inputScriptSize];
            toHex(script, inputScript, inputScriptSize);

            fprintf(inputFile, "%" PRIu64 ",%" PRIu64 ",\"%s\",\"%s\",%" PRIu64 "\n", txID, inputIndex, script, outputTxHash, outputIndex);
        }
    }

    virtual void wrapup()
    {
        fclose(outputFile);
        fclose(inputFile);
        fclose(blockFile);
        fclose(txFile);
        info("Done\n");
        exit(0);
    }
};

static CSVDump csvDump;

