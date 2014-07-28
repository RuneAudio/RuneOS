/* Copyright (C) 2014 J.F.Dockes
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the
 *	 Free Software Foundation, Inc.,
 *	 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _MPDCLI_H_X_INCLUDED_
#define _MPDCLI_H_X_INCLUDED_

#include <unordered_map>
#include <string>
#include <map>
#include <vector>

#include <regex.h>

class UpSong {
public:
    UpSong() : duration_secs(0), mpdid(0) {}
    void clear() {
        uri.clear(); 
        artist.clear(); 
        album.clear();
        title.clear();
        tracknum.clear();
        genre.clear();
        duration_secs = mpdid = 0;
    }
    std::string uri;
    std::string artist;
    std::string album;
    std::string title;
    std::string tracknum;
    std::string genre;
    unsigned int duration_secs;
    unsigned int mpdid;
};

class MpdStatus {
public:
    MpdStatus() : trackcounter(0), detailscounter(0) {}

    enum State {MPDS_UNK, MPDS_STOP, MPDS_PLAY, MPDS_PAUSE};

    int volume;
    bool rept;
    bool random;
    bool single;
    bool consume;
    int qlen;
    int qvers;
    State state;
    unsigned int crossfade;
    float mixrampdb;
    float mixrampdelay;
    int songpos;
    int songid;
    unsigned int songelapsedms; //current ms
    unsigned int songlenms; // song millis
    unsigned int kbrate;
    unsigned int sample_rate;
    unsigned int bitdepth;
    unsigned int channels;
    std::string errormessage;
    UpSong currentsong;
    UpSong nextsong;

    // Synthetized fields
    int trackcounter;
    int detailscounter;
};

struct mpd_song;

class MPDCli {
public:
    MPDCli(const std::string& host, int port = 6600, 
           const std::string& pass="");
    ~MPDCli();
    bool ok() {return m_ok && m_conn;}
    bool setVolume(int ivol, bool isMute = false);
    int  getVolume();
    bool togglePause();
    bool pause(bool onoff);
    bool play(int pos = -1);
    bool playId(int pos = -1);
    bool stop();
    bool next();
    bool previous();
    bool repeat(bool on);
    bool random(bool on);
    bool single(bool on);
    bool consume(bool on);
    bool seek(int seconds);
    bool clearQueue();
    int insert(const std::string& uri, int pos);
    // Insert after given id. Returns new id or -1
    int insertAfterId(const std::string& uri, int id);
    bool deleteId(int id);
    bool statId(int id);
    int curpos();
    bool getQueueData(std::vector<UpSong>& vdata);
    bool statSong(UpSong& usong, int pos = -1, bool isId = false);
    UpSong& mapSong(UpSong& usong, struct mpd_song *song);
    
    const MpdStatus& getStatus()
    {
        updStatus();
        return m_stat;
    }

private:
    void *m_conn;
    bool m_ok;
    MpdStatus m_stat;
    // Saved volume while muted.
    int m_premutevolume;
    // Volume that we use when MPD is stopped (does not return a
    // volume in the status)
    int m_cachedvolume; 
    std::string m_host;
    int m_port;
    std::string m_password;
    regex_t m_tpuexpr;

    bool openconn();
    bool updStatus();
    bool getQueueSongs(std::vector<mpd_song*>& songs);
    void freeSongs(std::vector<mpd_song*>& songs);
    bool showError(const std::string& who);
    bool looksLikeTransportURI(const std::string& path);
};


#endif /* _MPDCLI_H_X_INCLUDED_ */
