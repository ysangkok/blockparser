#!/bin/bash

# Update the blockchain database, resultant stats and output website
cd ~/blockparser/db

# Find ID of first block to read
function nextblock()
{
psql -q -t -h localhost -U blockchain <<-EOPSQL
SELECT max(f_id) + 1 FROM t_block
EOPSQL
}
FIRSTBLOCK=`nextblock`

~/blockparser/parser csvdump -f $FIRSTBLOCK

time psql -q -a -h localhost -U blockchain blockchain <<EOPSQL
\copy t_block from 'blocks.csv' WITH (FORMAT CSV, HEADER);
\copy t_transaction from 'transactions.csv' WITH (FORMAT CSV, HEADER);
\copy t_output from 'outputs.csv' WITH (FORMAT CSV, HEADER);
\copy t_input from 'inputs.csv' WITH (FORMAT CSV, HEADER);
EOPSQL

# Update resultant stats
function nextstats()
{
psql -q -t -h localhost -U blockchain <<-EOPSQL
SELECT (max(f_date) + interval '1 day')::date FROM t_daily
EOPSQL
}
FIRSTSTATS=`nextstats`

~/blockparser/db/dates.py $FIRSTSTATS | psql -q -t -h localhost -U blockchain

~/blockparser/db/dumpstats.sh

~/blockparser/db/sync.sh
