# Database configuration settings
MYSQL_CONFIG = {
    'host': 'localhost',
    'user': 'root',
    'password': '',
    'database': 'automated_pms'
}

def get_db_connection():
    import mysql.connector
    return mysql.connector.connect(**MYSQL_CONFIG)