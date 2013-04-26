
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
    uint64_t totalBlkOutput;
    uint64_t totalBlkFees;
    uint64_t numBlkTxs;

    const uint8_t *txStart;
    uint64_t numTxInputs;
    uint64_t numTxOutputs;

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
        fprintf(txFile, "ID,Hash,Version,BlockId,NumInputs,NumOutputs,LockTime,Size\n");

        blockFile = fopen("blocks.csv", "w");
        if(!blockFile) sysErrFatal("couldn't open file blocks.csv for writing\n");
        fprintf(blockFile, "ID,Hash,Timestamp,Version,Nonce,Difficulty,Merkle,NumTransactions,OutputValue,FeesValue,Size\n");

        inputFile = fopen("inputs.csv", "w");
        if(!inputFile) sysErrFatal("couldn't open file inputs.csv for writing\n");
        fprintf(inputFile, "ID,TransactionId,Index,OutputID,OutputIndex,Script\n");

        outputFile = fopen("outputs.csv", "w");
        if(!outputFile) sysErrFatal("couldn't open file outputs.csv for writing\n");
        fprintf(outputFile, "ID,TransactionId,Index,InputID,Value,Script,ReceivingAddress\n");

        /*
        FILE *bashFile = fopen("blockChain.bash", "w");
        if(!bashFile) sysErrFatal("couldn't open file blockChain.bash for writing\n");

        fprintf(
            bashFile,
            "\n"
            "#!/bin/bash\n"
            "\n"
            "echo\n"
            "\n"
            "echo 'wiping/re-creating DB blockChain ...'\n"
            "time mysql -u root -p -hlocalhost --password='' < blockChain.sql\n"
            "echo done\n"
            "echo\n"
            "\n"
            "for i in blocks inputs outputs transactions\n"
            "do\n"
            "    echo Importing table $i ...\n"
            "    time mysqlimport -u root -p -hlocalhost --password='' --lock-tables --use-threads=3 --local blockChain $i.txt\n"
            "    echo done\n"
            "    echo\n"
            "done\n"
            "\n"
        );
        fclose(bashFile);
        */

        return 0;
    }

    virtual void startBlock(
        const uint8_t *p
    )
    {
        fprintf(stderr, "Start block\n");
        if(0<=cutoffBlock && cutoffBlock<(int64_t)blkID) wrapup();

        blkStart = p;
        totalBlkOutput = 0;
        totalBlkFees = 0;
        numBlkTxs = 0;

        uint8_t blockHash[kSHA256ByteSize];
        sha256Twice(blockHash, p, 80);

        LOAD(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        LOAD(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        LOAD(uint32_t, difficulty, p);
        LOAD(uint32_t, nonce, p);

        // ID
        fprintf(blockFile, "%" PRIu64 ",", blkID++);

        // Hash
        uint8_t buf[1 + 2*kSHA256ByteSize];
        toHex(buf, blockHash);
        fprintf(blockFile, "%s,", buf);

        // Timestamp
        fprintf(blockFile, "%" PRIu64 ",", (uint64_t)blkTime);

        // Version
        fprintf(blockFile, "%" PRIu32 ",", version);

        // Nonce
        fprintf(blockFile, "%" PRIu32 ",", nonce);

        // Difficulty
        fprintf(blockFile, "%" PRIu32 ",", difficulty);

        // TODO Merkle root
        //uint8_t buf2[1 + 2*kSHA256ByteSize];
        //fprintf(blockFile, "%s,", buf);
    }

//    virtual void oldstartBlock(
//        const Block *b,
//        uint64_t
//    )
//    {
//        if(0<=cutoffBlock && cutoffBlock<b->height) wrapup();
//
//        uint8_t blockHash[kSHA256ByteSize];
//        sha256Twice(blockHash, b->data, 80);
//
//        const uint8_t *p = b->data;
//        LOAD(uint32_t, version, p);
//        SKIP(uint256_t, prevBlkHash, p);
//        LOAD(uint256_t, blkMerkleRoot, p);
//        LOAD(uint32_t, blkTime, p);
//        LOAD(uint32_t, difficulty, p);
//        LOAD(uint32_t, nonce, p);
//
//        // ID
//        fprintf(blockFile, "%" PRIu64 ",", (blkID = b->height-1));
//
//        // Hash
//        uint8_t buf[1 + 2*kSHA256ByteSize];
//        toHex(buf, blockHash);
//        fprintf(blockFile, "%s,", buf);
//
//        // Timestamp
//        fprintf(blockFile, "%" PRIu64 ",", (uint64_t)blkTime);
//
//        // Version
//        fprintf(blockFile, "%" PRIu32 ",", version);
//
//        // Nonce
//        fprintf(blockFile, "%" PRIu32 ",", nonce);
//
//        // Difficulty
//        fprintf(blockFile, "%" PRIu32 ",", difficulty);
//
//        // TODO Merkle root
//        //uint8_t buf2[1 + 2*kSHA256ByteSize];
//        //fprintf(blockFile, "%s,", buf);
//
//        // Nonce
//        fprintf(blockFile, "%" PRIu32 ",", nonce);
//
//    }

    virtual void endBlock(
        const uint8_t *p
    )
    {
        // Number of transactions
        fprintf(blockFile, "%" PRIu64 ",", numBlkTxs);

        // Value of output transactions
        fprintf(blockFile, "%" PRIu64 ",", totalBlkOutput);
        // TODO Value of fees
        fprintf(blockFile, "%" PRIu64 ",", totalBlkFees);

        // Size - off by 4?
        fprintf(blockFile, "%" PRIu64 "\n", (uint64_t)(p - blkStart));
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

        // ID
        fprintf(txFile, "%" PRIu64 ",", txID++);

        // Hash
        uint8_t buf[1 + 2*kSHA256ByteSize];
        toHex(buf, hash);
        fprintf(txFile, "%s,", buf);

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

        // Lock time
        LOAD(uint32_t, lockTime, p);
        fprintf(txFile, "%" PRIu32 ",", lockTime);

        // Size - off by 4?
        fprintf(txFile, "%" PRIu64 "\n", (uint64_t)(p - txStart));
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
        totalBlkOutput += value;

        // ID
        fprintf(outputFile, "%" PRIu64 ",", outputID);

        // Transaction ID
        fprintf(outputFile, "%" PRIu64 ",", txID);

        // Index
        fprintf(outputFile, "%" PRIu64 ",", outputIndex);

        // TODO Transaction input ID
        fprintf(outputFile, ",");

        // Value
        fprintf(outputFile, "%" PRIu64 ",", value);

        // TODO Script

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

        // TODO Script
        fprintf(inputFile, "\n");
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

