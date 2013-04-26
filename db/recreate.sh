#!/bin/sh

dropdb -h localhost -U blockchain blockchain
createdb -E UTF-8 -h localhost -U blockchain blockchain
psql -h localhost -U blockchain blockchain <blockchain.schema
