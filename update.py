import urllib.request
import re
import math

'''
Open an url and return its content as bare string.
'''
def fetch_webpage(url):
    response = urllib.request.urlopen(url)
    return str(response.read())

'''
Returns a list of all urls containing album data.
Might be slow, but tolerable, and
    keeps this form in case for future update.
'''
def source_urls():
    base = r'https://www.discogs.com/search/?limit=250&genre_exact=Classical&layout=sm'
    content = fetch_webpage(base)
    open("file.html", "w").write(content)
    pattern = re.compile("250 of (.*?)<", re.S)
    num_str = re.findall(pattern, content)[0]
    total = "".join(filter(str.isdigit, num_str))
    pages = math.ceil(int(total) / 250)
    return [(base + "&page=%d"%(i+1)) for i in range(pages)]



'''
Collects and processes data to be downloaded by a bunch of urls.
'''
def collect_data():
    pattern_str = 'class=\"card card_text-only float_fix.*?shortcut_navigable\"(.*?)'+\
			      '<a class=\"search_result_title \" href=\".*?\".*?>(.*?)</a>'
    pattern = re.compile(pattern_str, re.S)
    artist_pattern_str = '<a href="/artist/(.*?)\">'
    artist_pattern = re.compile(artist_pattern_str, re.S)
    urls = source_urls()
    artist_res = set()
    album_res = []
    url_count = 1
    for url in urls:
        print("Procesing URL " + str(url_count) + " out of " + str(len(urls)))
        url_count += 1
        content = fetch_webpage(url)
        albums = re.findall(pattern, content)
        for artist_str, album in albums:
            album_res.append(album + '\n')
            artists_raw = re.findall(artist_pattern, artist_str)
            artists = [(s + '\n') for s in artists_raw]
            artist_res.update(artists)
            album_res += artists
            album_res.append('.\n')
    return artist_res, album_res



'''
Update the current album and artist data,
    create one if don't exist.
Always starts over, because the database gets updated slowly
    so starting over should not be a huge overhead.
'''
def fetch_data():
    fartist = open("./data/Artist.txt", mode="w")
    falbum = open("./data/Album.txt", mode="w")
    artists, albums = collect_data()
    fartist.writelines(artists)
    falbum.writelines(albums)