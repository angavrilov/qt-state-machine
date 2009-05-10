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

#ifndef SPMODEL_H
#define SPMODEL_H
#include <QObject>
#include <QUrl>
#include <QAbstractItemModel>
#include "songdata.h"

class SPModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl currentSong READ currentSong)
    Q_PROPERTY(QString currentSongTitle READ currentSongTitle)
    Q_PROPERTY(QString currentSongArtist READ currentSongArtist)
    Q_PROPERTY(QString currentSongAlbum READ currentSongAlbum)
    Q_PROPERTY(int albumCount READ albumCount)


    public slots:
        void clearAlbumFilter ();
        void clearSongFilter ();
        void loadArtists ();
        void filterAlbumsByArtist(const QString & name);
        void loadGenres ();
        void filterSongsByGenre(const QString & genre);
        void loadPlaylists();
        void loadAlbums();
        void filterSongsByPlaylist(const QString & uid);
        void filterSongsByAlbum(const QString & name);
        void loadSongs ();
        void selectSong (const QString &);
        void gotoNext();
        void gotoPrev();
        void addSong (const SongData &);
        void reset ();

    signals:
            void albumChanged(const QString &);
            void artistChanged(const QString &);
            void genreChanged(const QString &);
            void songListChanged();
            void songChanged ();
            void endOfList ();
                        
    public:
        SPModel(QObject*);
        virtual ~SPModel ();

        QUrl currentSong();
        QString currentSongTitle ();
        QString currentSongArtist();
        QString currentSongAlbum();
        QAbstractItemModel* albumsItemModel() const;
        QAbstractItemModel* genresItemModel() const;
        QAbstractItemModel* songsItemModel() const;
        QAbstractItemModel* playlistsItemModel() const;
        QAbstractItemModel* artistsItemModel() const;
        int albumCount() const;

    private:
        class SPModelPvt* pvt;
};

#endif
