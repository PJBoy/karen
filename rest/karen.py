import bottle, json, subprocess, sys

@bottle.get()
def search():
    ret = []
    karen.stdin.write(f'{bottle.request.GET.q}\n')
    karen.stdin.flush()
    n_results = int(karen.stdout.readline())
    for _ in range(n_results):
        subtitles = []
        n_subtitles, episodeName = karen.stdout.readline().strip().split(', ')
        n_subtitles = int(n_subtitles)
        for _ in range(n_subtitles):
            t = karen.stdout.readline().strip()
            print(f"t = {episodeName}, {t}")
            time_begin, time_end, text = t.split(', ', 2)
            subtitles += [{'time_begin': int(time_begin), 'time_end': int(time_end), 'text': text}]
        
        karen.stdout.readline()
        ret += [{'episodeName': episodeName, 'subtitles': subtitles}]
        
    return json.dumps(ret)

@bottle.get()
def image():
    def formatTimestamp(milliseconds):
        return f"{milliseconds // 3600000}:{milliseconds // 60000 % 60}:{milliseconds // 1000 % 60}.{milliseconds % 1000}"
        
    return subprocess.run(f'ffmpeg -ss {formatTimestamp(int(bottle.request.GET.timestamp))} -i "{videoDirectory}/{bottle.request.GET.episodeName}.avi" -vframes 1 -f image2 -', stdout = subprocess.PIPE).stdout
    

if len(sys.argv) != 5:
    print("karen <karen filepath> <videos directory> <subtitles directory> <offsets filepath>")
    sys.exit(1)
    
karenFilepath, videoDirectory, subtitleDirectory, offsetsFilepath = sys.argv[1:]
karen = subprocess.Popen([karenFilepath, videoDirectory, subtitleDirectory, offsetsFilepath], stdin = subprocess.PIPE, stdout = subprocess.PIPE, universal_newlines = True)

bottle.run(host = '127.0.0.1', port = 8000)
