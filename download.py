import requests
import psycopg2
import re
import logging

# Configuration
RIOT_API_KEY = 'RGAPI-67f4436d-e015-46ba-9462-aea26aceee2f'
SUMMONER_NAME = 'MoonFloww'
SUMMONER_TAG = '1188'
REGION = 'euw1'
ROUTING = 'europe'

PG_CONFIG = {
    'host': 'localhost',
    'port': 5432,
    'dbname': 'leagueoflegends',
    'user': 'player',
    'password': 'xyz'
}

logging.basicConfig(level=logging.INFO)


def validate_api_key():
    url = f"https://{REGION}.api.riotgames.com/lol/status/v4/platform-data"
    r = requests.get(url, headers={'X-Riot-Token': RIOT_API_KEY})
    return r.status_code == 200


def get_puuid(game_name, tag_line):
    url = f"https://{ROUTING}.api.riotgames.com/riot/account/v1/accounts/by-riot-id/{game_name}/{tag_line}"
    headers = {'X-Riot-Token': RIOT_API_KEY}
    resp = requests.get(url, headers=headers)
    resp.raise_for_status()
    return resp.json()['puuid']


def get_match_ids(puuid, count=10):
    url = f"https://{ROUTING}.api.riotgames.com/lol/match/v5/matches/by-puuid/{puuid}/ids?count={count}"
    headers = {'X-Riot-Token': RIOT_API_KEY}
    resp = requests.get(url, headers=headers)
    resp.raise_for_status()
    return resp.json()


def get_match_details(match_id):
    url = f"https://{ROUTING}.api.riotgames.com/lol/match/v5/matches/{match_id}"
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
            gold_per_second INT,
            total_gold INT,
            current_gold INT,
            xp INT,
            level INT,
            jungle_minions_killed INT,
            minions_killed INT,
            time_enemy_spent_controlled INT,
            damage_dealt_to_champs INT,
            damage_taken INT,
            vision_score INT,
            items TEXT[]
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
                game_timestamp, gold_earned, gold_per_second, total_gold, current_gold,
                xp, level, jungle_minions_killed, minions_killed, time_enemy_spent_controlled,
                damage_dealt_to_champs, damage_taken, vision_score, items
            ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
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
                gold_per_second = EXCLUDED.gold_per_second,
                total_gold = EXCLUDED.total_gold,
                current_gold = EXCLUDED.current_gold,
                xp = EXCLUDED.xp,
                level = EXCLUDED.level,
                jungle_minions_killed = EXCLUDED.jungle_minions_killed,
                minions_killed = EXCLUDED.minions_killed,
                time_enemy_spent_controlled = EXCLUDED.time_enemy_spent_controlled,
                damage_dealt_to_champs = EXCLUDED.damage_dealt_to_champs,
                damage_taken = EXCLUDED.damage_taken,
                vision_score = EXCLUDED.vision_score,
                items = EXCLUDED.items
            '''
            items = [p.get(f'item{i}', 0) for i in range(7)]
            items_str = [str(i) for i in items]
            cur.execute(sql, (
                p['participantId'], p['summonerName'], p['role'], p['championId'], p['mirrorChampion'],
                p['teamKills'], p['enemyKills'], p['win'], p['kills'], p['deaths'], p['assists'],
                game_timestamp, p.get('goldEarned', 0), p.get('goldPerSecond', 0),
                p.get('totalGold', 0), p.get('currentGold', 0), p.get('xp', 0), p.get('level', 0),
                p.get('jungleMinionsKilled', 0), p.get('minionsKilled', 0),
                p.get('timeEnemySpentControlled', 0), p.get('totalDamageDealtToChampions', 0),
                p.get('totalDamageTaken', 0), p.get('visionScore', 0), items_str
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
        mirror = p['championId'] in (team_1_champions if p['teamId'] == 100 else team_0_champions)
        team_kills = team_0_kills if p['teamId'] == 100 else team_1_kills
        enemy_kills = team_1_kills if p['teamId'] == 100 else team_0_kills

        role_str = p.get('teamPosition') or 'UnknownRole'
        champion_str = str(p['championId'])

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
            'goldPerSecond': p.get('goldPerSecond', 0),
            'totalGold': p.get('totalGold', 0),
            'currentGold': p.get('currentGold', 0),
            'xp': p.get('xp', 0),
            'level': p.get('level', 0),
            'jungleMinionsKilled': p.get('jungleMinionsKilled', 0),
            'minionsKilled': p.get('minionsKilled', 0),
            'timeEnemySpentControlled': p.get('timeEnemySpentControlled', 0),
            'totalDamageDealtToChampions': p.get('totalDamageDealtToChampions', 0),
            'totalDamageTaken': p.get('totalDamageTaken', 0),
            'visionScore': p.get('visionScore', 0),
            **{f'item{i}': p.get(f'item{i}', 0) for i in range(7)},
            'table_name_part': f"{role_str}_{champion_str}_{'mirror' if mirror else 'nomirror'}_{team_kills}_{enemy_kills}_{'win' if p['win'] else 'loss'}"
        })

    parts = [p['table_name_part'] for p in processed_participants[:5]]
    table_name = 'game_' + '__'.join(parts)
    table_name = sanitize_table_name(table_name)

    return table_name, processed_participants, game_timestamp


def main():
    logging.info("Validating API key...")
    if not validate_api_key():
        logging.error("Invalid or expired API key.")
        return

    puuid = get_puuid(SUMMONER_NAME, SUMMONER_TAG)
    logging.info(f"PUUID for {SUMMONER_NAME}#{SUMMONER_TAG}: {puuid}")

    match_ids = get_match_ids(puuid, count=10)
    print("Recent match IDs:", match_ids)

    match_id = input("Enter match ID to analyze: ").strip()
    if not match_id.startswith("EUW1_"):
        match_id = f"EUW1_{match_id}"

    if match_id not in match_ids:
        print("Warning: Match ID not found in recent history. Proceeding anyway...")

    match_data = get_match_details(match_id)
    table_name, participants, game_timestamp = process_match_data(match_data)

    conn = psycopg2.connect(**PG_CONFIG)
    create_game_table(conn, table_name)
    save_participant_data(conn, table_name, participants, game_timestamp)

    logging.info(f"Saved data for match {match_id} in table '{table_name}'")
    conn.close()

if __name__ == '__main__':
    main()
