#!/bin/sh
cp  ~/Downloads/comp9315-assignment/pname.c .
cp  ~/Downloads/comp9315-assignment/pname.source .
cp  ~/Downloads/comp9315-assignment/data.sql .

make 
dropdb test
createdb test
psql test -f pname.sql
psql test -f data.sql