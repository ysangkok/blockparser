/* Count number and value of unspent transaction outputs */
SELECT COUNT(1), SUM(f_value) FROM t_output
WHERE f_inputtxhash IS NULL;
/*  5,107,067 | 11,101,064.80381183 */

/* Count number of unspent dust transaction outputs */
SELECT COUNT(1), SUM(f_value) FROM t_output
WHERE f_inputtxhash IS NULL
  AND f_value <= 5430;
/* 2,203,109 | 21.48020849 */

/* Count number of transactions which only have unspent dust transaction outputs */
SELECT COUNT(DISTINCT f_transactionid) FROM t_output
WHERE f_inputtxhash IS NULL
  AND f_value <= 5430;
/* 437102 */

/* Count number and value of spent dust transaction outputs */
SELECT COUNT(1), SUM(f_value) FROM t_output
WHERE f_inputtxhash IS NOT NULL
  AND f_value <= 5430;
/* 1,886,651 | 19.00621285*/

/* Count number of transactions which include dust outputs */
SELECT COUNT(1) FROM (
  SELECT DISTINCT f_transactionid
  FROM t_output
  WHERE f_value <= 5430) outputs;

/* Percentage of addresses used for more than one output */
SELECT COUNT(1)
FROM (SELECT COUNT(1) AS f_uses
      FROM t_output
      GROUP BY f_receivingaddress) d
WHERE d.f_uses > 1;

/* How big are the coins in UTXO */
SELECT f_bucket, AVG(f_value)::BIGINT
FROM (SELECT f_value, ntile(10) OVER(ORDER BY f_value) as f_bucket
      FROM t_output
      WHERE f_inputtxhash IS NULL) d
GROUP BY f_bucket
ORDER BY f_bucket;
/*
        1 |          1
        2 |         23
        3 |        469
        4 |       2153
        5 |       6753
        6 |      13303
        7 |      83002
        8 |    1010205
        9 |    8850930
       10 | 2163703493
*/

/* Block size distribution */
SELECT f_bucket, AVG(f_size)::BIGINT
FROM (SELECT f_size, ntile(10) OVER(ORDER BY f_size) as f_bucket
      FROM t_block) d
GROUP BY f_bucket
ORDER BY f_bucket;

/* Block numtransactions (recent) */
SELECT f_bucket, AVG(f_numtransactions)::BIGINT
FROM (SELECT f_numtransactions, ntile(10) OVER(ORDER BY f_numtransactions) as f_bucket
      FROM t_block
      WHERE f_id > 200000) d
GROUP BY f_bucket
ORDER BY f_bucket;

SELECT f_bucket, AVG(f_value)::BIGINT
FROM (SELECT f_value, ntile(10) OVER(ORDER BY f_value) as f_bucket
      FROM t_output
      WHERE f_inputtxhash IS NULL) d
GROUP BY f_bucket
ORDER BY f_bucket;
