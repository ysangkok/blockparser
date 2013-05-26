#!/bin/bash

function dumpstat()
{
psql -t -h localhost -U blockchain >data/${1}byday.json <<-EOPSQL
WITH r AS (SELECT f_date AS x, f_$1 AS y FROM t_daily ORDER BY f_date)
SELECT array_to_json(array_agg(r))
FROM r;
EOPSQL

psql -t -h localhost -U blockchain >data/${1}byweek.json <<-EOPSQL
WITH r AS (SELECT f_date AS x, f_$1 AS y FROM t_weekly ORDER BY f_date)
SELECT array_to_json(array_agg(r))
FROM r;
EOPSQL

psql -t -h localhost -U blockchain >data/${1}bymonth.json <<-EOPSQL
WITH r AS (SELECT f_date AS x, f_$1 AS y FROM t_monthly ORDER BY f_date)
SELECT array_to_json(array_agg(r))
FROM r;
EOPSQL
}

function dumpdistrib()
{
psql -t -h localhost -U blockchain >data/${1}.json <<-EOPSQL
SELECT array_to_json(f_$1) FROM t_daily ORDER BY f_date DESC LIMIT 1;
EOPSQL
}

dumpstat numblocks
dumpstat numtxs
dumpstat totalvalue
dumpstat totalfees
dumpstat avgtxsperblock
dumpstat avgfeesperblock
dumpstat avgblocksize
dumpstat numutxos
dumpdistrib utxonumdistrib
dumpdistrib utxovaluedistrib
dumpdistrib blocksizedistrib
dumpdistrib blocktimedistrib
