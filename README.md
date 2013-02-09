Welcome to the ScoreoidX for Cocos2d-x wiki!

ScoreoidX is a wrapper for using Scoreoid leaderboards in your project. The nice thing about Scoreoid is that the interface is using http/https calls and returns xml or json files. ScoreoidX uses rapidjson (included) to process the json results. As extra i have made a login/create player to link the scores to a specific device.

There is no need to install any additional sdks for Scoreoid.

Take a look at their website for more information and to register.

www.scoreoid.net

www.scoreoid.com

INSTALLATION:

Copy the following folders to the classes folder of your project: ScoreoidX rapidjson

If you want to use a "unique" login you could use the uniqueID generator which is included in the project.

Copy the following files to the ios folder of your project: ios\UIDevice ios\Crossplatform.mm (the objective c++ ==> c++ bridge)

And following file to the classes folder: classes\crossplatform.h

Include the Scoreoid.h file in the class where you want to use it, extend this class with ScoreoidDelegate. For example:

include "Scoreoid.h"
include "CrossPlatform.h"
class HelloWorld : public cocos2d::CCLayer, public ScoreoidDelegate { };

Add the callbacks to the header file:

// Scoreoid delegates
void scoreCallback(CCDictionary* returnDictionary,const char* apiCall,int& result);
void playerCallback(CCDictionary* returnDictionary,const char* apiCall,int& result);
void gameCallback(CCDictionary* returnDictionary,const char* apiCall,int& result);
void scoreoidAvailable(CCDictionary* returnDictionary,const char* apiCall,int& result);
And implement them in the implementation file, see HelloWorldScene.cpp for the examples. Initialize the scoreoid singleton:

/*
 * Scoreoid
 */

// Set the delegate 
Scoreoid::GetInstance()->setDelegate(this);
Use the following to login to Scoreoid:

// Login to Scoreoid using an unique identifier
Scoreoid::GetInstance()->login(CrossPlatform::Identifiers::uniqueID(), false);
