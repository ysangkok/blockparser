#!/bin/bash

dropdb -h localhost -U blockchain blockchain
createdb -E UTF-8 -h localhost -U blockchain blockchain
time psql -a -h localhost -U blockchain blockchain <~/blockparser/db/blockchain.schema
