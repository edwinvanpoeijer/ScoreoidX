#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

#include "Scoreoid.h"
#include "CrossPlatform.h"

class HelloWorld : public cocos2d::CCLayer, public ScoreoidDelegate
{
public:
    // Method 'init' in cocos2d-x returns bool, instead of 'id' in cocos2d-iphone (an object pointer)
    virtual bool init();

    // there's no 'id' in cpp, so we recommend to return the class instance pointer
    static cocos2d::CCScene* scene();
    
    // a selector callback
    void menuCloseCallback(CCObject* pSender);

    // preprocessor macro for "static create()" constructor ( node() deprecated )
    CREATE_FUNC(HelloWorld);
    
    // Scoreoid delegates
    void scoreCallback(CCDictionary* returnDictionary,const char* apiCall,int& result);
    void playerCallback(CCDictionary* returnDictionary,const char* apiCall,int& result);
    void gameCallback(CCDictionary* returnDictionary,const char* apiCall,int& result);
    void scoreoidAvailable(CCDictionary* returnDictionary,const char* apiCall,int& result);

};

#endif // __HELLOWORLD_SCENE_H__
