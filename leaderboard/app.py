import os
import psycopg2
from flask import Flask

app = Flask(__name__)

def get_db_connection():
    conn = psycopg2.connect(
        host=os.environ.get('DB_HOST', 'db'),
        database=os.environ.get('DB_NAME', 'snake_leaderboard'),
        user=os.environ.get('DB_USER', 'admin'),
        password=os.environ.get('DB_PASS', 'secret_snake_pass')
    )
    return conn

with app.app_context():
    try:
        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute('''
                    CREATE TABLE IF NOT EXISTS scores (
                        id SERIAL PRIMARY KEY,
                        nick VARCHAR(50) NOT NULL,
                        score INTEGER NOT NULL
                        )
                    ''')
        conn.commit()
        cur.close()
        conn.close()
        print("Baza danych zainicjalizowana pomyślnie!")
    except Exception as e:
        print(f"Błąd przy inicjalizacji bazy: {e}")

@app.route('/')
def index():
    try:
        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute('SELECT nick, score FROM scores ORDER BY score DESC LIMIT 10;')
        scores = cur.fetchall()
        cur.close()
        conn.close()

        html = "<h1 style='font-family: sans-serif;'>🐍 Globalna Tablica Wyników Snake 🐍</h1>"
        html += "<ul style='font-family: sans-serif; font-size: 20px;'>"
        for row in scores:
            html += f"<li><b>{row[0]}</b> - {row[1]} pkt</li>"
        html += "</ul>"

        if not scores:
            html += "<p style='font-family: sans-serif;'>Brak wyników. Rozbij się w grze, aby dodać pierwszy wynik!</p>"

        return html
    except Exception as e:
        return f"<h3>Trwa łączenie z bazą lub wystąpił błąd... Odśwież za chwilę.</h3><p>{str(e)}</p>"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)