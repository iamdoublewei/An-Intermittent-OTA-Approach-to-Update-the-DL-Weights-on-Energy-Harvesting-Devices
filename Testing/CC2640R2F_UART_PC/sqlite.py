import sqlite3
from sqlite3 import Error

class Sqlite:
    def __init__(self, db_file):
        self.conn = None
        self.connect(db_file)
        self.create()

    def connect(self, db_file):
        """
        create a database connection to the SQLite database specified by file
        :param db_file: database file directory
        """
        try:
            self.conn = sqlite3.connect(db_file)
            print('connected to database.')
        except Error as e:
            print(e)

    def create(self):
        """
        create a table from the sql statement
        :param sql: a CREATE TABLE statement
        """
        sql = '''CREATE TABLE IF NOT EXISTS leds (
                    id integer PRIMARY KEY,
                    timestamp integer NOT NULL,
                    system_time integer NOT NULL,
                    led0 integer NOT NULL,
                    led1 integer NOT NULL
                );'''

        try:
            cur = self.conn.cursor()
            cur.execute(sql)
        except Error as e:
            print(e)

    def insert(self, led):
        """
        create a new row of led into the leds table
        :param led: example: led = ('123', 'on');)
        :return: generated led id
        """
        sql = '''INSERT INTO leds(timestamp,system_time,led0,led1) VALUES(?,?,?,?)'''

        try:
            cur = self.conn.cursor()
            cur.execute(sql, led)
            self.conn.commit()
            return cur.lastrowid
        except Error as e:
            print(e)
        return None

    def select(self):
        """
        query all rows in the leds table
        """
        cur = self.conn.cursor()
        cur.execute("SELECT * FROM leds")
        rows = cur.fetchall()

        for row in rows:
            print(row)
