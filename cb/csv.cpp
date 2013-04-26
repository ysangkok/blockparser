
// Full CSV dump of the blockchain

#include <util.h>
#include <stdio.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <callback.h>

static uint8_t empty[kSHA256ByteSize] = { 0x42 };
typedef GoogMap<Hash256, uint64_t, Hash256Hasher, Hash256Equal>::Map OutputMap;

struct CSVDump:public Callback
{
    FILE *txFile;
    FILE *blockFile;
    FILE *inputFile;
    FILE *outputFile;

    uint64_t txID;
    uint64_t blkID;
    uint64_t inputID;
    uint64_t outputID;
    int64_t cutoffBlock;
    OutputMap outputMap;
    optparse::OptionParser parser;

    // Reference information
    const uint8_t *blkStart;
    uint64_t totalBlkInput;
    uint64_t totalBlkOutput;
    uint64_t numBlkTxs;

    const uint8_t *txStart;
    uint64_t numTxInputs;
    uint64_t numTxOutputs;
    uint64_t totalTxInput;
    uint64_t totalTxOutput;

    CSVDump()
    {
        parser
            .usage("[options] [list of addresses to restrict output to]")
            .version("")
            .description("create an CSV dump of the blockchain")
            .epilog("")
        ;
        parser
            .add_option("-a", "--atBlock")
            .action("store")
            .type("int")
            .set_default(-1)
            .help("stop dump at block <block> (default: all)")
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
        inputID = 0;
        outputID = 0;

        static uint64_t sz = 32 * 1000 * 1000;
        outputMap.setEmptyKey(empty);
        outputMap.resize(sz);

        optparse::Values &values = parser.parse_args(argc, argv);
        cutoffBlock = values.get("atBlock");

        info("dumping the blockchain ...");

        txFile = fopen("transactions.csv", "w");
        if(!txFile) sysErrFatal("couldn't open file txs.csv for writing\n");
        fprintf(txFile, "ID,Hash,Version,BlockId,NumInputs,NumOutputs,OutputValue,FeesValue,LockTime,Size\n");

        blockFile = fopen("blocks.csv", "w");
        if(!blockFile) sysErrFatal("couldn't open file blocks.csv for writing\n");
        fprintf(blockFile, "ID,Hash,Version,Timestamp,Nonce,Difficulty,Merkle,NumTransactions,OutputValue,FeesValue,Size\n");

        inputFile = fopen("inputs.csv", "w");
        if(!inputFile) sysErrFatal("couldn't open file inputs.csv for writing\n");
        fprintf(inputFile, "ID,TransactionId,Index,OutputID,OutputIndex,Script\n");

        outputFile = fopen("outputs.csv", "w");
        if(!outputFile) sysErrFatal("couldn't open file outputs.csv for writing\n");
        fprintf(outputFile, "ID,TransactionId,Index,Value,Script,ReceivingAddress\n");

        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t chainBase
    )
    {
        if(0<=cutoffBlock && cutoffBlock<b->height) wrapup();

        uint8_t blockHash[kSHA256ByteSize];
        sha256Twice(blockHash, b->data, 80);

        const uint8_t *p = b->data;

        blkStart = p;
        totalBlkInput = 0;
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
        //strftime(buf, sizeof tbuf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

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

    virtual void endBlock(
        const Block *b
    )
    {
        // Number of transactions
        fprintf(blockFile, "%" PRIu64 ",", numBlkTxs);

        // Value of output transactions
        fprintf(blockFile, "%" PRIu64 ",", totalBlkOutput);

        // Value of fees
        fprintf(blockFile, "%" PRIu64 ",", totalBlkInput - totalBlkOutput);

        // TODO Size - off by 4?
        fprintf(blockFile, "%" PRIu64 "\n", (uint64_t)(b->data - blkStart));

        blkID++;
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    )
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

    virtual void endTX(
        const uint8_t *p
    )
    {
        // Number of inputs
        fprintf(txFile, "%" PRIu64 ",", numTxInputs);

        // Number of outputs
        fprintf(txFile, "%" PRIu64 ",", numTxOutputs);

        // Value of outputs
        fprintf(txFile, "%" PRIu64 ",", totalTxOutput);

        // Value of fees
        fprintf(txFile, "%" PRIu64 ",", totalTxInput - totalTxOutput);

        // Lock time
        LOAD(uint32_t, lockTime, p);
        fprintf(txFile, "%" PRIu32 ",", lockTime);

        // Size - off by 4?
        fprintf(txFile, "%" PRIu64 "\n", (uint64_t)(p - txStart));

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
        numTxOutputs++;
        totalTxOutput += value;
        totalBlkOutput += value;

        // ID
        fprintf(outputFile, "%" PRIu64 ",", outputID);

        // Transaction ID
        fprintf(outputFile, "%" PRIu64 ",", txID);

        // Index
        fprintf(outputFile, "%" PRIu64 ",", outputIndex);

        // Value
        fprintf(outputFile, "%" PRIu64 ",", value);

        // Script
        fputc('"', outputFile);
        for (uint64_t i = 0; i < outputScriptSize; i++)
        {
          fprintf(outputFile, "%02x", outputScript[i]);
        }
        fputc('"', outputFile);
        fputc(',', outputFile);

        // Receiving address
        uint8_t address[40];
        address[0] = 'X';
        address[1] = 0;
        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(pubKeyHash.v, outputScript, outputScriptSize, addrType);
        if(likely(0<=type)) hash160ToAddr(address, pubKeyHash.v);
        fprintf(outputFile, "%s\n", address);

        // Store output so that we can look it up when it becomes an input
        uint32_t oi = outputIndex;
        uint8_t *h = allocHash256();
        memcpy(h, txHash, kSHA256ByteSize);
        uintptr_t ih = reinterpret_cast<uintptr_t>(h);
        uint32_t *h32 = reinterpret_cast<uint32_t*>(ih);
        h32[0] ^= oi;
        outputMap[h] = outputID++;
    }

    virtual void startInput(
        const uint8_t *p
    )
    {
        static uint256_t gNullHash;
        LOAD(uint256_t, upTXHash, p);
        if (0==memcmp(gNullHash.v, upTXHash.v, sizeof(gNullHash))) {
            uint64_t reward = getBaseReward(blkID);
            totalTxInput += reward;
            totalBlkInput += reward;
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
        numTxInputs++;
        totalTxInput += value;
        totalBlkInput += value;

        // ID
        fprintf(inputFile, "%" PRIu64 ",", inputID++);

        // Transaction ID
        fprintf(inputFile, "%" PRIu64 ",", txID);

        // Index
        fprintf(inputFile, "%" PRIu64 ",", inputIndex);

        // Transaction output ID
        uint256_t h;
        uint32_t oi = outputIndex;
        memcpy(h.v, upTXHash, kSHA256ByteSize);
        uintptr_t ih = reinterpret_cast<uintptr_t>(h.v);
        uint32_t *h32 = reinterpret_cast<uint32_t*>(ih);
        h32[0] ^= oi;
        auto src = outputMap.find(h.v);
        if(outputMap.end()==src) errFatal("unconnected input");
        fprintf(inputFile, "%" PRIu64 ",", src->second);

        // Transaction output index
        fprintf(inputFile, "%" PRIu64 ",", outputIndex);

        // Script
        fputc('"', inputFile);
        for (uint64_t i = 0; i < inputScriptSize; i++)
        {
          fprintf(inputFile, "%02x", inputScript[i]);
        }
        fputc('"', inputFile);
        fputc('\n', inputFile);
    }

    virtual void wrapup()
    {
        fclose(outputFile);
        fclose(inputFile);
        fclose(blockFile);
        fclose(txFile);
        info("done\n");
        exit(0);
    }
};

static CSVDump csvDump;

