These files are intended to be used with xu4 (http://xu4.sourceforge.net/) to replace the "tracker-based" music provided in the xu4 package.

This is the music from Ultima IV:  Quest of the Avatar as played back by a Roland SC-55st.  Original music composed by Kenneith W. Arnold.  Special Roland Sound Canvas-oriented General MIDI conversion originally accomplished by Telavar at Mysterious Sosaria (http://www.surfing.net/ultima/index.html).

Instructions

1.  Place the all the music tracks located in the "mid" directory into your "xu4/mid/"
2.  Edit the "xu4/conf/music.xml" file appropriately to point to the "ogg" version of the track.  Or, overwrite the music.xml file with the one provided (in the "conf" directory).
3.  Done!


Notes

xu4 will actually play back just about any file that you put in the "mid" directory and point to appropriately in the music.xml file.  However, I noticed some issues when using various types of files.  For example, using MP3 files with variable bit rates caused some "squirking" sounds during xu4 playback that was not present when played normally in Windows.  Also, track looping in-game was delayed and therefore jarring to the listener.  Original/raw WAV file in-game playback was better, but the file size was quite large and for some reason xu4 introduced artifacts to the "shopping" track playback (again, not audible during standard out-of-game playback).  Eventually, OGG was settled upon as the best compromise since it introduced no delays, had a reasonable filesize, and the "shopping" track has less artifacting than the raw WAV file.

--Hurin
http://www.dor-lomin.com
7/14/2013


Here is Telavar's original README.TXT for the MIDI files themselves

Ultima IV - Quest of the Avatar
Midi music for the Roland Soundcanvas

All rights reserved by Origin Systems Inc. 1985
respectively the original composers


Compiled & arranged by

--Telavar

http://www.surfing.net/ultima/
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Mysterious Sosaria   

mailto:telavar@surfing.net