clickr
------

This is my own simple upload tool for Flickr. Written in idle time
over a weekend or two in April 2011, it uploads one file, as specified
on the command line, to my photostream.

The application requires an API key and a shared secret, which can be
gotten from Flickr: http://www.flickr.com/services/api/keys/
These attributes are stored in ~/.clickr. The format of this
configuration file is:

	api_key = "<api_key>";
	secret = "<shared_secret>";

With these in place, run ./clickr -a and follow the instructions to
generate the authorisation token. Once this is complete, upload photos
by running:
./clickr -f filename [-t title] [-d description]

building
========

	cd _build
	cmake ..
	make

The build requires at least the following libraries that are
probably installed on your system, but might not be:
	 libconfig
	 libssl
	 libcurl

Any other questions: sdstrowes@gmail.com

Stephen Strowes, April 2011.
