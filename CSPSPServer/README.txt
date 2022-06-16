===============================================================================
  CSPSP Server                                                     Kevin Chen
  version 1.51b                                                          2011		
  
  official site: http://cspsp.appspot.com
  forums:        http://z4.invisionfree.com/CSPSP
===============================================================================

This the server application for CSPSP, a homebrew game for the Sony PSP.

-------------------------------------------------------------------------------
  INSTALLATION/SETUP
-------------------------------------------------------------------------------

This application requires the Microsoft .NET Framework. Please make sure you
have it installed before continuing (http://www.microsoft.com/net).

1. Extract the CSPSPServer folder to any location on your harddrive
2. Open config.txt to customize your server's settings (see the following
   section for more information).
3. If you are behind a router, portforward the port specified in config.txt
   (default is 42692). Check out this very helpful tutorial by xXxSpectre@:
   http://z4.invisionfree.com/CSPSP/index.php?showtopic=150.
4. Run CSPSPServer.exe! If everything went smoothly, you should now be able to
   connect to your server from your psp.
   

-------------------------------------------------------------------------------
  CONFIGURATION
-------------------------------------------------------------------------------

To configure your server, open up data/config.txt. Inside, you'll find a few 
settings that you can change. They should be pretty self-explanatory, but here
are a few short descriptions:

"name"           - the name of your server (max 32 characters)
"autobalance"    - enables/disables team auto-balance ["on"/"off"]
"friendlyfire"   - enables/disables friendly fire ["on"/"off"]
"alltalk"        - enables/disables chat between dead players/spectators and 
                    players still alive ["on"/"off"]
"maxplayers"     - the maximum number of players that can join the server 
                    (max 32)
"roundtime"      - the duration of a round in seconds (minimum 10)
"freezetime"     - the freeze time before a round in seconds (max 10)
"buytime"        - the time in the beginning of a round allowed to buy weapons
"maptime"        - the duration of a map in minutes (maximum 120)
"port"           - the port that the server will use (between 1024 and 65536)
"respawntime"    - the time it takes someone to respawn after dying in seconds
                    (max 30)
"spawngun"       - the index of the gun you want players to spawn with in ctf
                    and ffa (for example, 11 is the MP5; -1 is just the 
                    default pistols)
"invincibletime" - the time a player is invincible for after respawning in 
                     seconds (max 10)

To change the map cycle, open up data/mapcycle.txt. List the maps in the order 
that you want them to cycle in, in the format [map name] [type] on each line, 
where type is either "tdm", "ctf", or "ffa".

To manually ban players, open up data/banlist.txt and enter their name*, with 
one name per line.

To add admins, open up data/admins.txt and enter their name*, with one name 
per line. Admins basically have access to the same commands as the server 
owner (such as kick, ban, etc), except they can do so remotely while in-game.

You can also modify the data/guns.txt, although extreme values for any of the 
gun properties might not work well online.
  
* name refers to the player's account name, without the clan tag. For example,
  the account name of someone named "[clan]name" would just be "name".


-------------------------------------------------------------------------------
  COMMANDS
-------------------------------------------------------------------------------

Commands can be entered in the input line (indicated by ">") at the bottom of 
the server, Here's a list of the available commands:

/help                 - lists available commands and their arguments
/timeleft             - shows remaining time left for current map
/kick [name]          - kicks player with specified name*
/ban [name]           - bans+kicks player with specified name*
/unban [name]         - unbans player with specified name*
/map [mapname] [type] - changes to a new map (type can either be tdm, ctf or 
                        ffa; default is tdm)
/resetround           - starts a new round and resets scores
normal text           - sends a server message to all players

* name refers to the player's account name, without the clan tag. For example,
  the account name of someone named "[clan]name" would just be "name".


-------------------------------------------------------------------------------
  COMMON ERRORS
-------------------------------------------------------------------------------

Here is a list of a few error messages and their explanations/solutions:

"Error: Map could not be loaded" 
   - this probably means that the map itself has problems.
   
"Error registering server: Server already registered"
   - the server was not unregistered correctly the last time it was closed. 
     This is more of a warning than an error; the server will still function.
   
"Error registering server: Version outdated"
   - a new version of the server application is available. It's recommended 
     that you update as soon as possible.
   
"Error contacting master server"
   - the master server that holds the list of servers is unavailable 
     (or your internet connection isn't working). The server will still run, 
     and players who have your server saved in favorites might be able to 
     still connect.
   
"Error registering server: Supplied IP does not match"
   - the IP sent to the master server differs from the IP that the master 
     server sees. This error is usually uncommon; try restarting the server 
     application.


-------------------------------------------------------------------------------
  UPDATES
-------------------------------------------------------------------------------

-added support for CSPSP version 1.91
-fixed freezing bug from v1.50
-tweaked networking code to be more robust (and caught a few bugs); also fixed 
  major exploits (speedhacking, infinite ammo, etc.)
-added a time slowdown effect at the end of a round
