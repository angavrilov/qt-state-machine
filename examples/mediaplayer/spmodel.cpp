/****************************************************************************
**
** This file is part of a Qt Solutions component.
** 
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
** 
** Contact:  Qt Software Information (qt-info@nokia.com)
** 
** Commercial Usage  
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
** 
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
** 
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
** 
** GNU General Public License Usage 
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
** 
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
** 
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** 
****************************************************************************/

#include "spmodel.h"
#include <QSqlQueryModel>
#include <QtSql>
#include <QStandardItemModel>
#include <QMessageBox>
// an SQL query model that always as column (0) as uid and column (1) as display
class SPSqlQueryModel : public QSqlQueryModel
{
    Q_OBJECT
    public:
    SPSqlQueryModel (QObject* o = NULL): QSqlQueryModel (o) {}
    virtual QVariant data(const QModelIndex & index, int role) const
    {
        QModelIndex idx(index);
        if (role == Qt::DisplayRole && query().record().count() > 1) {
            idx = idx.sibling(idx.row(),1);
        } else if (role == Qt::UserRole)
            role = Qt::DisplayRole;
        return QSqlQueryModel::data(idx,role);
    }
};


class SPModelPvt
{
    public:
    SPSqlQueryModel 
                    artistModel, 
                    albumModel, 
                    songModel,
                    playlistModel, genreModel;

    QSqlQuery artistQuery, albumQuery, songQuery, playlistQuery, genreQuery, playingQuery;

};

SPModel::SPModel(QObject* o)
    :QObject(o)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("sqlite.db");
    if (!db.open()) {
        QMessageBox::critical(0, qApp->tr("Cannot open database"),
            qApp->tr("Unable to establish a database connection.\n"
                     "This example needs SQLite support. Please read "
                     "the Qt SQL driver documentation for information how "
                     "to build it.\n\n"
                     "Click Cancel to exit."), QMessageBox::Cancel);
    }

    db.exec("CREATE TABLE IF NOT EXISTS songs (song_url VARCHAR(1024) PRIMARY KEY, song_title VARCHAR(1024), song_artist VARCHAR(1024), song_album VARCHAR(1024), song_track_index SMALLINT)");
    db.exec("DROP TABLE genres");
    db.exec("CREATE TABLE IF NOT EXISTS genres (genre_title VARCHAR(64), genre_song_url VARCHAR(1024))");
//    db.exec("CREATE TABLE IF NOT EXISTS playlists (playlist_id VARACHAR(256) PRIMARY KEY, playlist_title VARCHAR(1024))");
//    db.exec("CREATE TABLE IF NOT EXISTS playlist_songs (playlist_song_id BIGINT PRIMARY KEY, playlist_song_playlist

    pvt = new SPModelPvt;
    pvt->artistQuery = QSqlQuery("SELECT DISTINCT song_artist FROM songs");
    pvt->albumQuery = QSqlQuery("SELECT DISTINCT song_album FROM songs");
    pvt->playlistQuery = QSqlQuery("SELECT playlist_url, playlist_title FROM playlists");
    pvt->songQuery = QSqlQuery("SELECT song_url, song_title from songs");
    pvt->genreQuery = QSqlQuery("SELECT DISTINCT genre_title from genres");
    QSqlQuery q;
//    q.exec("SELECT DISTINCT song_url,song_title FROM songs, genres WHERE genre_song_url=song_url AND genre_title='All'");
    q.exec("SELECT * from genres");
    while (q.next()) {
        qDebug () << q.value(0);
    }
}

void SPModel::addSong ( const SongData & data)
{

    QSqlQuery q;
    q.prepare("SELECT count(*) FROM songs WHERE song_url=:url");
    q.bindValue(":url",data.url);
    bool inserting = true;
    if (q.exec()) {
        q.next();
        inserting = q.value(0).toInt() == 0;
    }
    if (inserting) {
        q.prepare ("INSERT INTO songs (song_url, song_title, song_artist, song_album, song_track_index) VALUES (:url, :title, :artist, :album, :track)");
    } else {
        q.prepare("UPDATE songs SET song_title=:title, song_album=:album, song_track_index=:track WHERE song_url=:url ");
    }
    q.bindValue(":url",data.url);
    q.bindValue(":title",data.title);
    q.bindValue(":artist",data.artist);
    q.bindValue(":album",data.album);
    q.bindValue(":track",data.trackNumber);
    q.exec();

    q.prepare ("DELETE FROM genres WHERE genre_song_url=:url");
    q.bindValue(":url",data.url);
    q.exec();

    q.prepare ("INSERT INTO genres (genre_song_url, genre_title) VALUES(:url, :genre)");
    q.bindValue(":url",data.url);
    QStringList gn = data.genres;
    gn << "All";
    foreach (QString g, gn) {
        q.bindValue(":genre",g);
        q.exec ();
    }

    if (inserting) {
        emit albumChanged(data.album);
        emit songListChanged();
        emit artistChanged(data.artist);
        foreach (QString g, data.genres) {
            emit genreChanged(g);
        }
    }
    

}
  

SPModel::~SPModel()
{
    delete pvt;
}

int SPModel::albumCount() const
{
    return pvt->albumModel.rowCount();
}

void SPModel::clearAlbumFilter ()
{
    pvt->albumQuery = QSqlQuery ("SELECT DISTINCT song_album FROM songs ");
}
void SPModel::clearSongFilter ()
{
    pvt->songQuery = QSqlQuery ("SELECT song_url, song_title FROM songs");
}
void SPModel::loadArtists ()
{
    pvt->artistQuery.exec ();
    pvt->artistModel.setQuery(pvt->artistQuery);
}
void SPModel::filterAlbumsByArtist(const QString & artist)
{
    pvt->albumQuery.prepare("SELECT DISTINCT song_album FROM songs WHERE song_artist=:artist");
    pvt->albumQuery.bindValue(":artist",artist);
}
void SPModel::filterSongsByAlbum(const QString & album)
{
    pvt->albumQuery.prepare("SELECT song_url,song_title, song_track_index FROM songs WHERE song_album=:album ORDER BY song_track_index");
    pvt->albumQuery.bindValue(":album",album);
}
void SPModel::loadGenres ()
{
    pvt->genreQuery.exec();
    pvt->genreModel.setQuery(pvt->genreQuery);
}

void SPModel::filterSongsByGenre(const QString & genre)
{
    pvt->songQuery.prepare ("SELECT DISTINCT song_url,song_title FROM songs, genres WHERE genre_song_url=song_url AND genre_title=:genre");
    pvt->songQuery.bindValue(":genre",genre);
}
void SPModel::loadPlaylists()
{
    pvt->playlistQuery.exec ();
    pvt->playlistModel.setQuery(pvt->playlistQuery);
}
void SPModel::loadAlbums()
{
    pvt->albumQuery.exec ();
    pvt->albumModel.setQuery(pvt->albumQuery);
}
void SPModel::filterSongsByPlaylist(const QString & uid)
{
    pvt->songQuery.prepare("SELECT DISTINCT song_url, song_title, playlist_song_index FROM playlist_songs INNER JOIN songs ON playlist_song_url=song_url WHERE playlist_id=:playlist ORDER BY playlist_song_index");
    pvt->songQuery.bindValue(":playlist",uid);
}
void SPModel::loadSongs ()
{
    pvt->songQuery.exec ();
    pvt->songModel.setQuery(pvt->songQuery);
}
QUrl SPModel::currentSong()
{
    if (pvt->playingQuery.isValid())
        return QUrl(pvt->playingQuery.value(0).toString());
    else
        return QUrl();
}
QString SPModel::currentSongTitle()
{
    if (pvt->playingQuery.isValid())
        return pvt->playingQuery.value(1).toString();
    else
        return QString();
}
QString SPModel::currentSongArtist()
{
    QSqlQuery q;
    q.prepare("SELECT song_artist FROM songs WHERE song_url=:url");
    q.bindValue(":url",currentSong().toString());
    q.exec();
    q.next();
    return q.value(0).toString();
}
QString SPModel::currentSongAlbum()
{
    QSqlQuery q;
    q.prepare("SELECT song_album FROM songs WHERE song_url=:url");
    q.bindValue(":url",currentSong().toString());
    q.exec();
    q.next();
    return q.value(0).toString();
}

void SPModel::selectSong (const QString & s)
{
    pvt->playingQuery = QSqlQuery(pvt->songQuery.executedQuery ());

    while (pvt->playingQuery.next()) {
        if (pvt->playingQuery.value(0).toString() == s) {
            emit songChanged ();
            return;
        }
    }
    emit endOfList ();
}

void SPModel::reset ()
{
    pvt->playingQuery = QSqlQuery(pvt->songQuery.executedQuery ());
    pvt->playingQuery.exec();
}

void SPModel::gotoNext()
{
    if (pvt->playingQuery.next()) {
        emit songChanged ();
    }else
        emit endOfList ();
}
void SPModel::gotoPrev()
{
    if (pvt->playingQuery.previous())
        emit songChanged ();
    else
        emit endOfList ();
}



QAbstractItemModel* SPModel::albumsItemModel() const
{
    return &pvt->albumModel;
}
QAbstractItemModel* SPModel::genresItemModel() const
{
    return &pvt->genreModel;
}
QAbstractItemModel* SPModel::songsItemModel() const
{
    return &pvt->songModel;
}
QAbstractItemModel* SPModel::playlistsItemModel() const
{
    return &pvt->playlistModel;
}
QAbstractItemModel* SPModel::artistsItemModel() const
{
    return &pvt->artistModel;
}

#include <spmodel.moc>
