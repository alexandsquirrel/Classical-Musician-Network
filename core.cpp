#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <queue>
#include <set>
#include <windows.h>
#include <chrono>
#include <random>
using namespace std;

struct Album
{
    Album(string a, vector<int> b):
        name(a), cast(b) {}
    // Name of the album, not its musicians.
    string name;
    // The ids of its featured artists.
    vector<int> cast;
};

// Abuse of global variables! But should not be an issue considering the scale 
// of this program.

// A map from the artists' names to their ids (for manipulation in the graph).
map<string,int> artists;
// A list of the artists' names.
vector<string> artistsList;
// A list of albums.
vector<Album> albums;
// The graph itself. Can be interpreted as a "dynamic 2d array".
vector<vector<pair<int,int>>> g;
// Temporary variable to record whether each node has been visited.
vector<bool> vis;
// To record ancestors of nodes on a path.
vector<int> anc, anc_album;
// Queue for BFS.
queue<int> q;
set<string> albumNames;
// Whether or not to see the next answer.
bool will;
int src,dest;

// Loads the artists' names into the array.
void loadArtists()
{
    ifstream is("./data/artist.txt");
    string name;
    int idx = 0;
    while(getline(is, name))
    {
        if(name == "Various?anv=" || name == "No+Artist?anv=") // Deals with minor faliures of the database.
            continue;
        artists.insert({name, idx++});
        artistsList.push_back(name);
    }
    cout << "Loaded " << artistsList.size() << " artists." << endl;
}

// Loads and stores the album as an array of Album s.
void loadAlbums()
{
    ifstream is("./data/album.txt");
    while(is)
    {
        string name,buf;
        vector<int> cast;
        getline(is, name);
        while(is)
        {
            is >> buf;
            if(buf == ".") break;
            if(buf != "Various?anv=" && buf != "No+Artist?anv=") // Filters out bad case.
                cast.push_back(artists[buf]);
        }
        albums.push_back(Album(name, cast));
        albumNames.insert(name);
        string nextline; // Filters out the extra whitespace ('\n').
        getline(is, nextline);
    }
    cout << "Loaded " << albums.size() << " albums." << endl;
}

// Set up the graph.
void setupGraph()
{
    for(int i = 0; i < artists.size(); ++i)
    {
        anc.push_back(-1);
        anc_album.push_back(-1);
        vis.push_back(0);
    }
    for(int i = 0; i < artists.size(); ++i)
        g.push_back(vector<pair<int, int>>()); // Initializes the empty graph.
    for(int aln = 0; aln < albums.size(); ++aln)
    {
        Album& album = albums[aln];
        vector<int> nodes; // To collect the interconnected musicians in this album.
        for(int p: album.cast) nodes.push_back(p);
        for(int i = 0; i < nodes.size(); ++i)
            for(int j = 0; j < i; ++j)
            {
                g[nodes[i]].push_back({nodes[j],aln});
                g[nodes[j]].push_back({nodes[i],aln});
            }
    }
}

/* Returns the cleaned-up form of a name.
   Example: 837573-Krystian-Zimerman -> Krystian Zimerman */
string guessName(const string &alias)
{
    int i;
    for(i = 0; i < alias.size() ; ++i)
        if(alias[i] == '-') break;
    string res = alias.substr(i+1,alias.size()-i-1);
    for(i = 0; i < res.size(); ++i)
        if(res[i] == '-') res[i] = ' ';
    return res;
}

// Prompts the user to search for an artist, and return its index.
int findArtist()
{
    string key;
    int result = -1;
    while(result == -1)
    {
        cout << "Input '/' to abort." << endl;
        cout << "Please input keyword (no space): ";
        cin >> key;
        if(key == "/") return -1;
        transform(key.begin(), key.end(), key.begin(),
                  [](char c) {return tolower(c);}
                  );
        vector<int> candidates;
        for(int i = 0; i < artistsList.size(); ++i)
        {
            // Finds all possible matches in the list of artists.
            // Since this is not an exact word match, no benefit at using a map.
            auto it = search(artistsList[i].begin(), artistsList[i].end(),
                            key.begin(), key.end(), 
                            [](char a,char b) {return tolower(a) == b;}
                            );
            if(it != artistsList[i].end()) // Got a result!
                candidates.push_back(i);
        }
        if(candidates.empty())
        {
            cout << "oops, no search result!" << endl;
            continue;
        }
        cout << "There are " << candidates.size() << " candidates." << endl;
        cout << "Select one, or type 0 to restart a search." << endl;
        for(int i = 0; i < candidates.size(); ++i)
            cout << (i+1) << ". " << guessName(artistsList[candidates[i]]) << endl;
        int decision;
        cin >> decision;
        result = decision? candidates[decision - 1] : (-1);
    }
    return result;
}

// Prints the path when BFS gots to an answer.
void printPath()
{
    cout << "Found a path!\n" << endl;
    stack<pair<int, int>> path;
    int cur = dest;
    while(true)
    {
        // Backtracing the path!
        path.push({cur, anc_album[cur]});
        if(anc[cur] == -1) break;
        cur = anc[cur];
    }
    cout << guessName(artistsList[path.top().first]) << endl;
    path.pop();
    while(!path.empty())
    {
        auto p = path.top();
        path.pop();
        int cur = p.first,albumidx = p.second;
        for(int i = 0; i < 3; ++i) cout << "            |" << endl;
        cout << "            | " << albums[albumidx].name << endl;
        for(int i = 0; i < 3; ++i) cout << "            |" << endl;
        cout << guessName(artistsList[cur]) << endl;
    }
    cout << "see other paths for this pair? (1=yes,0=no): ";
    cin >> will;
}

// Random engine.
int seed = chrono::system_clock :: now().time_since_epoch().count();
auto rndeng = default_random_engine(seed);

// Performs BFS to find a path.
// Once got a path, prints it.
void bfs(int cur)
{
    vector<int> buf;
    if(cur != dest)
        for(auto& p:g[cur]) if(!vis[p.first])
        {
            vis[p.first] = 1;
            anc[p.first] = cur;
            anc_album[p.first] = p.second;
            buf.push_back(p.first);
        }
    shuffle(buf.begin(),buf.end(),rndeng);
    for(int n : buf) q.push(n);
    if(cur == dest) printPath();
}

// The main workflow of handling a request.
bool handleRequest()
{
    cout << "What are the two artists?\n" << endl;
    src = findArtist();
    if(src == -1) return 0;
    dest = findArtist();
    if(dest == -1) return 0;
    will = 1;
    while(will)
    {
        q.push(src);
        vis[src] = 1;
        while(!q.empty())
        {
            bfs(q.front());
            q.pop();
            if(!will) break;
        }
        while(!q.empty()) q.pop();
        for(int i = 0; i < artists.size(); ++i)
        {
            vis[i] = 0;
            anc[i] = -1;
            anc_album[i] = -1;
        }
    }
    cout << "That's all!" << endl;
    return 1;
}

int main()
{
    ios :: sync_with_stdio(0); // To boost io speed.
    loadArtists();
    loadAlbums();
    setupGraph();
    cout << "--------------------" << endl;
    while(handleRequest()) ;
    return 0;
}
