
CREATE DATABASE IF NOT EXISTS test;

USE test;

SELECT 'natural test (n1-n5)' as '';

################# Word segmentation test #############################

DROP TABLE IF EXISTS t;
CREATE TABLE t (id INT, c VARCHAR(255), FULLTEXT (c) WITH PARSER thaift_parser) ENGINE=MyISAM CHARACTER SET utf8 COLLATE utf8_unicode_ci;
INSERT INTO t(id, c) VALUES 
  (1, 'กฎหมาย'),
  (2, 'มีหมาอยู่ในบ้าน');
SELECT id as 'n1 expect 2' FROM t WHERE MATCH(c) AGAINST('หมา'); # expect (2)


################# English alphabets test #############################

DROP TABLE IF EXISTS t;
CREATE TABLE t (id INT, c VARCHAR(255), FULLTEXT (c) WITH PARSER thaift_parser) ENGINE=MyISAM CHARACTER SET utf8 COLLATE utf8_unicode_ci;
INSERT INTO t(id, c) VALUES 
  (1, 'cat'),
  (2, 'rat');
SELECT id as 'n2 expect 1' FROM t WHERE MATCH(c) AGAINST('cat'); # expect (1)


################# Quote test #############################

DROP TABLE IF EXISTS t;
CREATE TABLE t (id INT, c VARCHAR(255), FULLTEXT (c) WITH PARSER thaift_parser) ENGINE=MyISAM CHARACTER SET utf8 COLLATE utf8_unicode_ci;
INSERT INTO t(id, c) VALUES 
  (1, '"กา"'),
  (2, '"มา"');
SELECT id as 'n3 expect 1' FROM t WHERE MATCH(c) AGAINST('กา'); # expect (1)


################# English and Thai test #############################

DROP TABLE IF EXISTS t;
CREATE TABLE t (id INT, c VARCHAR(255), FULLTEXT (c) WITH PARSER thaift_parser) ENGINE=MyISAM CHARACTER SET utf8 COLLATE utf8_unicode_ci;
INSERT INTO t(id, c) VALUES 
  (1, 'บ้าน(house)'),
  (2, 'บ้าน');
SELECT id as 'n4 expect 1' FROM t WHERE MATCH(c) AGAINST('house'); # expect (1)

############## Thai with number test ##############################

DROP TABLE IF EXISTS t;
CREATE TABLE t (id INT, c VARCHAR(255), FULLTEXT (c) WITH PARSER thaift_parser) ENGINE=MyISAM CHARACTER SET utf8 COLLATE utf8_unicode_ci;
INSERT INTO t(id, c) VALUES 
  (1, '1.บ้าน'),
  (2, '2.ไก่');
SELECT id as 'n5 expect 1' FROM t WHERE MATCH(c) AGAINST('บ้าน'); # expect (1)
