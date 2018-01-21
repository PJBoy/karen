import requests

url_search = 'http://127.0.0.1:8000/search'
url_image = 'http://127.0.0.1:8000/image'
proxies = {}

def get_search(quote):
    r = requests.get(url_search, proxies = proxies, params = {'q': quote})
    if not r.ok:
        raise RuntimeError(f'{r.status_code} - {r.reason} - {r.text}')
    return r.json()

def get_image(episode, timestamp):
    r = requests.get(url_image, proxies = proxies, params = {'episodeName': episode, 'timestamp': timestamp})
    if not r.ok:
        raise RuntimeError(f'{r.status_code} - {r.reason} - {r.text}')
    return r.content

episodes = get_search("Goo Lagoon")
firstEpisodeResult = episodes[0]
episodeName = firstEpisodeResult['episodeName']
firstSubtitleResult = firstEpisodeResult['subtitles'][0]

image = get_image(episodeName, firstSubtitleResult['time_begin'])
with open('temp.jpg', 'wb') as f:
    f.write(image)
