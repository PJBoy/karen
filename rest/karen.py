import bottle, json, os, subprocess, sys

def formatTimestamp(milliseconds):
    return f"{milliseconds // 3600000}:{milliseconds // 60000 % 60}:{milliseconds // 1000 % 60}.{milliseconds % 1000}"

@bottle.get()
def search():
    print(dict(bottle.request.GET))
    bottle.response.content_type = 'application/json'
    ret = []
    karen.stdin.write(f'{bottle.request.GET.q}\n')
    karen.stdin.flush()
    n_results = int(karen.stdout.readline())
    for _ in range(n_results):
        subtitles = []
        similarity = float(karen.stdout.readline())
        episodeName = karen.stdout.readline().strip()
        t = karen.stdout.readline().strip()
        karen.stdout.readline()
        print(f"similarity = {similarity}, t = {episodeName}, {t}")
        time_begin, time_end, text = t.split(', ', 2)
        ret += [{'similarity': similarity, 'episodeName': episodeName, 'time_begin': int(time_begin), 'time_end': int(time_end), 'text': text}]

    return json.dumps(ret)

@bottle.get()
def image():
    print(dict(bottle.request.GET))
    bottle.response.content_type = 'image/jpeg'
    args = [f'ffmpeg', '-hide_banner', '-ss', f'{formatTimestamp(int(bottle.request.GET.timestamp))}', '-i', os.path.join(videoDirectory, f'{bottle.request.GET.episodeName}.avi'), '-vframes', '1', '-f', 'image2', '-']
    print(' '.join(args))
    return subprocess.run(args, stdout = subprocess.PIPE).stdout

@bottle.get()
def video():
    print(dict(bottle.request.GET))
    filename = f'{bottle.request.GET.episodeName}.{bottle.request.GET.timestamp}.webm'
    args = [f'ffmpeg', '-hide_banner', '-ss', f'{formatTimestamp(int(bottle.request.GET.timestamp))}', '-i', os.path.join(videoDirectory, f'{bottle.request.GET.episodeName}.avi'), '-t', '0:0:10', '-vcodec', 'libvpx-vp9', '-acodec', 'libvorbis', '-preset', 'ultrafast', '-cpu-used', '-5', '-deadline', 'realtime', '-n', filename]
    print(' '.join(args))
    subprocess.run(args)
    return filename

@bottle.get()
def clear_cache():
    directoryPath = os.getcwd()
    for filename in os.listdir(directoryPath):
        if filename.endswith('.webm'):
            os.remove(os.path.join(directoryPath, filename))

@bottle.route('/<filename:re:.*\.webm>')
def video_url(filename):
    return bottle.static_file(filename, root = '')

if len(sys.argv) != 5:
    print("karen <karen filepath> <videos directory> <subtitles directory> <offsets filepath>")
    sys.exit(1)

karenFilepath, videoDirectory, subtitleDirectory, offsetsFilepath = sys.argv[1:]
karen = subprocess.Popen([karenFilepath, videoDirectory, subtitleDirectory, offsetsFilepath], stdin = subprocess.PIPE, stdout = subprocess.PIPE, universal_newlines = True)

bottle.run(host = '0.0.0.0', port = 8000)
