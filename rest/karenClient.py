import os, subprocess
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

while True:
    query = input('Query: ')
    if not query:
        break
    
    results = get_search(query)
    if len(results) == 0:
        print('No results')
        continue
    
    firstResult = results[0]
    episodeName = firstResult['episodeName']
    timestamp = firstResult['time_begin']
    print(f'Episode name: {episodeName}, similarity: {firstResult["similarity"]}, timestamp range: {timestamp}..{firstResult["time_end"]}')
    print(f'Text: {firstResult["text"]}')
    print()

    image = get_image(episodeName, timestamp)
    with open('temp.jpg', 'wb') as f:
        f.write(image)

    subprocess.call(f'rundll32 "C:\Program Files (x86)\Windows Photo Viewer\PhotoViewer.dll", ImageView_Fullscreen {os.path.abspath("temp.jpg")}')
