/* Count number and value of unspent transaction outputs */
SELECT COUNT(1), SUM(f_value) FROM t_output
WHERE f_inputtxhash IS NULL;
/* 5,003,212  11,090,539.80501183 */

/* Count number of unspent dust transaction outputs */
SELECT COUNT(1), SUM(f_value) FROM t_output
WHERE f_inputtxhash IS NULL
  AND f_value <= 5430;
/* 2,108,980 20.54413176 */

/* Count number of transactions which only have unspent dust transaction outputs */
SELECT COUNT(DISTINCT f_transactionid) FROM t_output
WHERE f_inputtxhash IS NULL
  AND f_value <= 5430;
/* 435608 */

/* Count number and value of spent dust transaction outputs */
SELECT COUNT(1), SUM(f_value) FROM t_output
WHERE f_inputtxhash IS NOT NULL
  AND f_value <= 5430;
/* 1,860,808 18.45227768 */

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
