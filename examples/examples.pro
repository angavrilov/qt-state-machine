TEMPLATE      = subdirs
SUBDIRS       = \
                clockticking \
                composition \
                eventtransitions \
                factorial \
                helloworld \
                pauseandresume \
                pingpong \
                trafficlight \
                twowaybutton \
		calc \
		blackjack
		
contains(QT_CONFIG,phonon) {
	SUBDIRS += mediaplayer
}
