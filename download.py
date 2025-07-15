import requests
import psycopg2
import re

# Configuration
RIOT_API_KEY = 'YOUR_RIOT_API_KEY'
PG_CONFIG = {
    'host': 'localhost',
    'port': 5432,
    'dbname': 'riot_db',
    'user': 'postgres',
    'password': 'your_password'
}
REGION = 'europe'  # For match-v5, region routing is different, use americas/europe/asia/etc.

def get_match_details(match_id):
    url = f'https://{REGION}.api.riotgames.com/lol/match/v5/matches/{match_id}'
    headers = {'X-Riot-Token': RIOT_API_KEY}
    resp = requests.get(url, headers=headers)
    resp.raise_for_status()
    return resp.json()

def sanitize_table_name(name):
    return re.sub(r'[^a-zA-Z0-9_]', '_', name)

def create_game_table(conn, table_name):
    with conn.cursor() as cur:
        sql = f'''
        CREATE TABLE IF NOT EXISTS "{table_name}" (
            participant_id INT PRIMARY KEY,
            summoner_name VARCHAR(100),
            role VARCHAR(50),
            champion_id INT,
            mirror_champion BOOLEAN,
            team_kills INT,
            enemy_kills INT,
            win BOOLEAN,
            kills INT,
            deaths INT,
            assists INT,
            game_timestamp BIGINT,
            gold_earned INT,
            damage_dealt_to_champs INT,
            damage_taken INT,
            total_minions_killed INT,
            vision_score INT
        )
        '''
        cur.execute(sql)
        conn.commit()

def save_participant_data(conn, table_name, participants, game_timestamp):
    with conn.cursor() as cur:
        for p in participants:
            sql = f'''
            INSERT INTO "{table_name}" (
                participant_id, summoner_name, role, champion_id, mirror_champion,
                team_kills, enemy_kills, win, kills, deaths, assists,
                game_timestamp, gold_earned, damage_dealt_to_champs,
                damage_taken, total_minions_killed, vision_score
            ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            ON CONFLICT (participant_id) DO UPDATE SET
                summoner_name = EXCLUDED.summoner_name,
                role = EXCLUDED.role,
                champion_id = EXCLUDED.champion_id,
                mirror_champion = EXCLUDED.mirror_champion,
                team_kills = EXCLUDED.team_kills,
                enemy_kills = EXCLUDED.enemy_kills,
                win = EXCLUDED.win,
                kills = EXCLUDED.kills,
                deaths = EXCLUDED.deaths,
                assists = EXCLUDED.assists,
                game_timestamp = EXCLUDED.game_timestamp,
                gold_earned = EXCLUDED.gold_earned,
                damage_dealt_to_champs = EXCLUDED.damage_dealt_to_champs,
                damage_taken = EXCLUDED.damage_taken,
                total_minions_killed = EXCLUDED.total_minions_killed,
                vision_score = EXCLUDED.vision_score
            '''
            cur.execute(sql, (
                p['participantId'],
                p['summonerName'],
                p['role'],
                p['championId'],
                p['mirrorChampion'],
                p['teamKills'],
                p['enemyKills'],
                p['win'],
                p['kills'],
                p['deaths'],
                p['assists'],
                game_timestamp,
                p.get('goldEarned', 0),
                p.get('totalDamageDealtToChampions', 0),
                p.get('totalDamageTaken', 0),
                p.get('totalMinionsKilled', 0),
                p.get('visionScore', 0)
            ))
        conn.commit()

def process_match_data(match_data):
    participants = match_data['info']['participants']
    game_timestamp = match_data['info']['gameStartTimestamp']

    team_0_champions = [p['championId'] for p in participants if p['teamId'] == 100]
    team_1_champions = [p['championId'] for p in participants if p['teamId'] == 200]

    team_0_kills = sum(p['kills'] for p in participants if p['teamId'] == 100)
    team_1_kills = sum(p['kills'] for p in participants if p['teamId'] == 200)

    processed_participants = []

    for p in participants:
        mirror = False
        if p['teamId'] == 100:
            mirror = p['championId'] in team_1_champions
            team_kills = team_0_kills
            enemy_kills = team_1_kills
        else:
            mirror = p['championId'] in team_0_champions
            team_kills = team_1_kills
            enemy_kills = team_0_kills

        role_str = p['teamPosition'] or 'UnknownRole'
        champion_str = str(p['championId'])
        mirror_str = 'mirror' if mirror else 'nomirror'
        teamkill_str = str(team_kills)
        enemykill_str = str(enemy_kills)
        winloss_str = 'win' if p['win'] else 'loss'

        processed_participants.append({
            'participantId': p['participantId'],
            'summonerName': p['summonerName'],
            'role': role_str,
            'championId': p['championId'],
            'mirrorChampion': mirror,
            'teamKills': team_kills,
            'enemyKills': enemy_kills,
            'win': p['win'],
            'kills': p['kills'],
            'deaths': p['deaths'],
            'assists': p['assists'],
            'goldEarned': p.get('goldEarned', 0),
            'totalDamageDealtToChampions': p.get('totalDamageDealtToChampions', 0),
            'totalDamageTaken': p.get('totalDamageTaken', 0),
            'totalMinionsKilled': p.get('totalMinionsKilled', 0),
            'visionScore': p.get('visionScore', 0),
            'table_name_part': f"{role_str}_{champion_str}_{mirror_str}_{teamkill_str}_{enemykill_str}_{winloss_str}"
        })

    parts = [p['table_name_part'] for p in processed_participants[:5]]
    table_name = 'game_' + '__'.join(parts)
    table_name = sanitize_table_name(table_name)

    return table_name, processed_participants, game_timestamp



def main():
    match_id = input("Enter match ID: ")

    match_data = get_match_details(match_id)

    table_name, participants = process_match_data(match_data)

    conn = psycopg2.connect(**PG_CONFIG)

    create_game_table(conn, table_name)

    save_participant_data(conn, table_name, participants)

    print(f"Saved data for match {match_id} in table '{table_name}'")

    conn.close()


if __name__ == '__main__':
    main()
