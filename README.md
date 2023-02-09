# MVDHIGHLIGHTS
Mvdhighlights is a tool which takes skuller's mvdtool strings output of a mvd demo, parses it, and generates a highlights.cfg file which can be executed inside q2pro to show you the highlight plays of a player in a AQ2 match in-game.

# Usage
Mvdhighlights takes the output of mvdtool through stdin and takes the player name as a parameter to output the highlights.cfg file. For example,
    
    mvdtool.exe strings pickup_wolk.mvd2 | mvdhighlights.exe -p mikota
    
If you have a space or weird symbols in your name, you can run it like this:

    mvdtool.exe strings pickup_wolk.mvd2 | mvdhighlights.exe -p '>fs> mikota'

The program will generate a highlights.cfg file which should then be copied into your action folder. Once there, load up the demo and type

    exec highlights
    
in the console and it should work.
