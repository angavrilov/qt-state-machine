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

#include "spview.h"
#include "spmodel.h"
#include <QDebug>
#include <QItemDelegate>

class SPViewPvt
{
    public:
    SPModel* model;
};


class SPItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    SPItemDelegate(QObject* o) : QItemDelegate(o) {}

    virtual void paint (QPainter* p, QStyleOptionViewItem & option, const QModelIndex & index) const
    {
        QString disp = index.data(Qt::DisplayRole).toString();
        drawBackground(p,option,index);
        drawDisplay(p,option,option.rect,disp);
    }
};

void SPView::setModel (SPModel* m)
{
    pvt->model = m;    
}

void SPView::showAlbums ()
{
    listView->setModel (pvt->model->albumsItemModel());
}

void SPView::showArtists ()
{
    listView->setModel (pvt->model->artistsItemModel());
}

void SPView::showGenres ()
{
    QAbstractItemModel* model = pvt->model->genresItemModel();
    listView->setModel (model);
}

void SPView::showSongs ()
{
    listView->setModel (pvt->model->songsItemModel());
}

void SPView::showPlaylists()
{
    listView->setModel (pvt->model->playlistsItemModel());
}


SPView::SPView(QWidget* w) : QWidget (w)
{
    pvt = new SPViewPvt;
    setupUi(this);
    listView->setItemDelegate(new SPItemDelegate(this));
}

QString SPView::currentItem() const
{
    QVariant v = listView->model()->data(listView->currentIndex(),Qt::UserRole);
    if (v.isNull())
       v = listView->currentIndex().data(Qt::DisplayRole);
    return v.toString ();
}

int SPView::itemCount () const
{
    return listView->model()->rowCount ();
}
int SPView::currentIndex() const
{
    return listView->currentIndex().row();
}

void SPView::setTotalTime (int t)
{
    posSlider->setMaximum(t);
}
void SPView::setCurrentTime (int t)
{
    posSlider->setValue (t);
}

SPView::~SPView ()
{
    delete pvt;
}
#include "spview.moc"
