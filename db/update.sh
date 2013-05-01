#!/bin/bash

time psql -a -h localhost -U blockchain blockchain <<EOPSQL
\copy t_block from 'blocks.csv' WITH (FORMAT CSV, HEADER);
\copy t_transaction from 'transactions.csv' WITH (FORMAT CSV, HEADER);
\copy t_output from 'outputs.csv' WITH (FORMAT CSV, HEADER);
\copy t_input from 'inputs.csv' WITH (FORMAT CSV, HEADER);
EOPSQL
