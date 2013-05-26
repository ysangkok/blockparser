#!/bin/bash

# Update the blockchain database, resultant stats and output website
cd ~/blockparser/db

# Find ID of first block to read
function nextblock()
{
psql -t -h localhost -U blockchain <<-EOPSQL
SELECT max(f_id) + 1 FROM t_block
EOPSQL
}
FIRSTBLOCK=`nextblock`

~/blockparser/parser csvdump -f $FIRSTBLOCK

time psql -a -h localhost -U blockchain blockchain <<EOPSQL
\copy t_block from 'blocks.csv' WITH (FORMAT CSV, HEADER);
\copy t_transaction from 'transactions.csv' WITH (FORMAT CSV, HEADER);
\copy t_output from 'outputs.csv' WITH (FORMAT CSV, HEADER);
\copy t_input from 'inputs.csv' WITH (FORMAT CSV, HEADER);
EOPSQL

# Update resultant stats
function nextstats()
{
psql -t -h localhost -U blockchain <<-EOPSQL
SELECT (max(f_date) + interval '1 day')::date FROM t_daily
EOPSQL
}
FIRSTSTATS=`nextstats`

~/blockparser/db/dates.py $FIRSTSTATS | psql -t -h localhost -U blockchain

~/blockparser/db/dumpstats.sh
rsync -avHz --delete data/ ks2.mcdee.net:/home/www/www.mcdee.net/bitcoin/data/

NEXTMONTH=`~/blockparser/db/nextmonth.py`
s3cmd sync -m application/json --add-header="Expires: ${NEXTMONTH} 01:00:00 GMT" data/*bymonth.json s3://charts.bconomy.com/data/

NEXTWEEK=`~/blockparser/db/nextweek.py`
s3cmd sync -m application/json --add-header="Expires: ${NEXTWEEK} 01:00:00 GMT" data/*byweek.json s3://charts.bconomy.com/data/

TOMORROW=`~/blockparser/db/tomorrow.py`
s3cmd sync -m application/json --add-header="Expires: ${TOMORROW} 01:00:00 GMT" data/*byday.json s3://charts.bconomy.com/data/
s3cmd sync -m application/json --add-header="Expires: ${TOMORROW} 01:00:00 GMT" data/*distrib s3://charts.bconomy.com/data/
