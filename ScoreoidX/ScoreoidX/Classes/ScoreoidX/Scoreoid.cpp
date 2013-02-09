/****************************************************************************
Copyright (c) 2013      Edwin van Poeijer

http://www.vanpoeijer.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "Scoreoid.h"

USING_NS_CC;
USING_NS_CC_EXT;


#define SCOREOID_GAME_ID "YOUR GAME IDE"
#define SCOREOID_API_KEY "YOUR API KEY"
#define SCOREOID_RESPONSE "json"
#define SCOREOID_POSTDATA "api_key=" SCOREOID_API_KEY "&game_id=" SCOREOID_GAME_ID "&response=" SCOREOID_RESPONSE

Scoreoid* Scoreoid::instance = NULL;


Scoreoid::~Scoreoid()
{
    CC_SAFE_RELEASE(this->apiDictionary);
}

Scoreoid* Scoreoid::GetInstance()
{
    if ( instance == NULL ) {
        instance = Scoreoid::create();
        instance->retain();
    }
    
    return instance;
}

Scoreoid::Scoreoid()
{
    this->m_delegate = NULL;
    this->_gameInProgress = false;
    this->_playerInProgress = false;
    this->_scoreInProgress = false;
    
    this->_scoreoidAvailable = false;
    this->_userAuthenticated = false;
    this->loginStatus = IDLE;
    
    // Load the API
    this->apiDictionary = CCDictionary::createWithContentsOfFile("scoreoidApi.plist");
    this->apiDictionary->retain();

}

/*
 * Remove empty fields from the API string
 */
const char* Scoreoid::removeEmptyFields(const char* value1, std::string delimiter)
{
    std::string value = value1;
    CCString* newString = CCString::create("");
    int currentPosition = 0;
    unsigned foundPosition = 0;
    
    while(foundPosition!=std::string::npos)
    {
        foundPosition = value.find(delimiter,currentPosition);
        std::string substring = value.substr (currentPosition,(foundPosition-currentPosition));
        char lastChar = substring[substring.length()-1];
        if (lastChar != '=')
        {
            CCString* original = newString;
            if (currentPosition ==0)
            {
                newString = CCString::createWithFormat("%s",substring.c_str());
                CCLog("FIELD:%s",newString->getCString());
            }
            else
            {
                newString = CCString::createWithFormat("%s&%s",original->getCString(),substring.c_str());
                CCLog("FIELD:%s",newString->getCString());
            }
        }
        currentPosition = foundPosition+1;
    }
    return newString->getCString();
}

Scoreoid * Scoreoid::create(void)
{
	Scoreoid * pRet = new Scoreoid();
	pRet->autorelease();
	return pRet;
}

/*
 * Process the JSON data, RapidJSON is used for this
 */
CCDictionary* Scoreoid::processJSON(const char *jsonData,const char* apiCall, int& result)
{
    CCDictionary* resultDictionary = CCDictionary::create();
    CCArray* resultArray = CCArray::create();
    
    // Set API call
    resultDictionary->setObject(CCString::create(apiCall), "api");
    // Document for json parsing
    Document document;
    
    if(!document.Parse<0>(jsonData).HasParseError())
    {
        
        // Check for error
        if (document.IsObject() && document.HasMember("error"))
        {
            resultDictionary->setObject(CCInteger::create(SO_API_ERROR), "status");
            resultDictionary->setObject(CCString::create(document["error"].GetString()), "message");
            result = SO_API_ERROR;
            return resultDictionary;
        }
        
        // Check if this is a simple response (only one keypair)
        CCDictionary* apiDict = (CCDictionary*)this->apiDictionary->objectForKey(apiCall);
        
        CCString* oneItemString = ((CCString*)apiDict->objectForKey("oneItem"));
        int oneItem = atoi(oneItemString->getCString());
        if (oneItem == 1)
        {
            CCDictionary* dict = CCDictionary::create();
            CCArray* itemNames = (CCArray*) apiDict->objectForKey("item");
            for (int counter = 0; counter < itemNames->count();counter++)
            {
                const char* fieldName = ((CCString*) itemNames->objectAtIndex(counter))->getCString();
                if (document.HasMember(fieldName))
                {
                    // Check for string
                    if(document[fieldName].IsString())
                        dict->setObject(CCString::create(document[fieldName].GetString()), fieldName);
                    else if(document[fieldName].IsInt())
                        dict->setObject(CCString::createWithFormat("%d",document[fieldName].GetInt()),fieldName);
                    else if(document[fieldName].IsInt64())
                        dict->setObject(CCString::createWithFormat("%d",document[fieldName].GetInt64()),fieldName);
                    else if(document[fieldName].IsUint())
                        dict->setObject(CCString::createWithFormat("%d",document[fieldName].GetUint()),fieldName);
                    else if(document[fieldName].IsUint64())
                        dict->setObject(CCString::createWithFormat("%d",document[fieldName].GetUint64()),fieldName);
                    else if(document[fieldName].IsBool())
                        dict->setObject(CCString::createWithFormat("%d",document[fieldName].GetBool()),fieldName);
                    else if(document[fieldName].IsDouble())
                        dict->setObject(CCString::createWithFormat("%f",document[fieldName].GetDouble()),fieldName);
                    else if(document[fieldName].IsNull())
                        dict->setObject(CCString::create(""),fieldName);
                }
            }
            resultDictionary->setObject(CCInteger::create(SO_API_SUCCES), "status");
            resultDictionary->setObject(CCString::create("success"), "message");
            resultArray->addObject(dict);
            resultDictionary->setObject(resultArray,"result");
            result = SO_API_SUCCES;
            return resultDictionary;
        }
    
        CCArray* allKeys = apiDict->allKeys();
        
        // Iterate through all objects
        for (Value::ConstValueIterator itr = document.Begin(); itr != document.End(); ++itr)
        {
            // Create dictionary object
            CCDictionary* scoreDict = CCDictionary::create();
            // Check for object (player, score)
            for (int objectIterator = 0; objectIterator < allKeys->count();objectIterator++)
            {
                // Get the objectName
                const char* objectName = ((CCString*)allKeys->objectAtIndex(objectIterator))->getCString();
                //CCLog("Objectname:%s",objectName);
                // Get all the itemnames
                CCArray* objectItems = (CCArray*) apiDict->objectForKey(objectName);
                
                // Check if the field matches
                if(itr->HasMember(objectName))
                {
                    // Iterate through all the members of this object
                    for (Value::ConstMemberIterator m = itr->MemberBegin(); m != itr->MemberEnd();++m)
                    {
                        Value::Member* member = (Value::Member*)m;
                        bool runAction = false;
                        runAction =  (std::strcmp(member->name.GetString(), objectName) == 0);
                        // Check if the fields of member are in our APIdictionary
                        for (int objectCounter = 0 ; runAction && objectCounter < objectItems->count(); objectCounter++)
                        {
                            const char* fieldName = ((CCString*) objectItems->objectAtIndex(objectCounter))->getCString();
                            
                            // Check if the field exists
                            if (member->value.HasMember(fieldName))
                            {
                                //CCLog("Adding field:%s",fieldName);
                                // Check for string
                                if(member->value[fieldName].IsString())
                                    scoreDict->setObject(CCString::create(member->value[fieldName].GetString()), fieldName);
                                else if(member->value[fieldName].IsInt())
                                    scoreDict->setObject(CCString::createWithFormat("%d",member->value[fieldName].GetInt()),fieldName);
                                else if(member->value[fieldName].IsInt64())
                                    scoreDict->setObject(CCString::createWithFormat("%d",member->value[fieldName].GetInt64()),fieldName);
                                else if(member->value[fieldName].IsUint())
                                    scoreDict->setObject(CCString::createWithFormat("%d",member->value[fieldName].GetUint()),fieldName);
                                else if(member->value[fieldName].IsUint64())
                                    scoreDict->setObject(CCString::createWithFormat("%d",member->value[fieldName].GetUint64()),fieldName);
                                else if(member->value[fieldName].IsBool())
                                    scoreDict->setObject(CCString::createWithFormat("%d",member->value[fieldName].GetBool()),fieldName);
                                else if(member->value[fieldName].IsDouble())
                                    scoreDict->setObject(CCString::createWithFormat("%f",member->value[fieldName].GetDouble()),fieldName);
                                else if(member->value[fieldName].IsNull())
                                    scoreDict->setObject(CCString::create(""),fieldName);
                            }
                        }
                    }
                }                
            }
            resultArray->addObject(scoreDict);
        }
        resultDictionary->setObject(CCInteger::create(SO_API_SUCCES), "status");
        resultDictionary->setObject(CCString::create("success"), "message");
        resultDictionary->setObject(resultArray, "result");
        result = SO_API_SUCCES;
    }
    else
    {
        resultDictionary->setObject(CCInteger::create(SO_API_ERROR), "status");
        resultDictionary->setObject(CCString::create("parse error"), "message");
        result = SO_API_ERROR;
        return resultDictionary;
    }
    
    
    return resultDictionary;
}

CCString* Scoreoid::getStringJSON(Value::Member* iterator, const char* field)
{
    if (iterator->value.HasMember(field))
        if (iterator->value[field].IsString())
            return CCString::create(iterator->value[field].GetString());
    
    return CCString::create("");
}

void Scoreoid::HttpRequest(const char* apiUrl,const char* data, const char* tag,SEL_CallFuncND pSelector)
{
    CCHttpRequest* request = new CCHttpRequest();
    
    request->setUrl(apiUrl);
    request->setRequestType(CCHttpRequest::kHttpPost);
    std::vector<std::string> headers;
    headers.push_back("application/x-www-form-urlencoded; charset=utf-8");
    request->setHeaders(headers);
    request->setResponseCallback(this, pSelector);
    
    // write the post data
    
    std::string endData = this->removeEmptyFields(data, "&");
    std::string scoreoid_postdat = SCOREOID_POSTDATA;
    std::string postData;
    if (strlen(endData.c_str()) > 0)
    {
         postData = scoreoid_postdat + "&" + endData;
    }
    else
    {
        postData = scoreoid_postdat;
    }
    CCLog("Posting:%s",postData.c_str());
    request->setRequestData(postData.c_str(), strlen(postData.c_str()));
    request->setTag(tag);
    CCHttpClient::getInstance()->send(request);
    request->release();
}

void Scoreoid::HttpRequestScoreCallback(cocos2d::CCNode *sender, void *data)
{
    CCDictionary *tmpArray = NULL;
    int result = SO_API_FAIL;
    CCHttpResponse *response = (CCHttpResponse*)data;
    
    if (!response)
    {
        
        return;
    }
//    if (0 != strlen(response->getHttpRequest()->getTag()))
//    {
//        //CCLog("%s completed", response->getHttpRequest()->getTag());
//    }
    
    int statusCode = response->getResponseCode();
    char statusString[64] = {};
    sprintf(statusString, "HTTP Status Code: %d, tag = %s", statusCode, response->getHttpRequest()->getTag());
    
    if (!response->isSucceed())
    {
        result = SO_API_FAIL;
    }
    else
    {
        // dump data and convert to string
        std::vector<char> *buffer = response->getResponseData();
        std::string s(buffer->begin(), buffer->end());
        tmpArray = this->processJSON(s.c_str(), response->getHttpRequest()->getTag(), result);
    }
    
    this->_scoreInProgress = false;
    
    // Callback
    if (ScoreoidDelegate* delegate = Scoreoid::GetInstance()->getDelegate()) {
        return delegate->scoreCallback(tmpArray,response->getHttpRequest()->getTag(),result);
    }

}
void Scoreoid::HttpRequestPlayerCallback(cocos2d::CCNode *sender, void *data)
{
    CCDictionary* tmpArray = NULL;
    int result = SO_API_FAIL;
    CCHttpResponse *response = (CCHttpResponse*)data;
    
    if (!response)
    {
        
        return;
    }
//    if (0 != strlen(response->getHttpRequest()->getTag()))
//    {
//        //CCLog("%s completed", response->getHttpRequest()->getTag());
//    }
    
    int statusCode = response->getResponseCode();
    char statusString[64] = {};
    sprintf(statusString, "HTTP Status Code: %d, tag = %s", statusCode, response->getHttpRequest()->getTag());
    
    if (!response->isSucceed())
    {
        result = SO_API_FAIL;
    }
    else
    {
        // dump data and convert to string
        std::vector<char> *buffer = response->getResponseData();
        std::string s(buffer->begin(), buffer->end());
        tmpArray = this->processJSON(s.c_str(), response->getHttpRequest()->getTag(), result);        
    }
    
    // Callback
    if (ScoreoidDelegate* delegate = Scoreoid::GetInstance()->getDelegate()) {
        return delegate->playerCallback(tmpArray,response->getHttpRequest()->getTag(),result);
    }
}
void Scoreoid::HttpRequestGameCallback(cocos2d::CCNode *sender, void *data)
{
    CCDictionary* tmpArray = NULL;
    int result = SO_API_FAIL;
    CCHttpResponse *response = (CCHttpResponse*)data;
    
    if (!response)
    {
        
        return;
    }
    if (0 != strlen(response->getHttpRequest()->getTag()))
    {
        //CCLog("%s completed", response->getHttpRequest()->getTag());
    }
    
    int statusCode = response->getResponseCode();
    char statusString[64] = {};
    sprintf(statusString, "HTTP Status Code: %d, tag = %s", statusCode, response->getHttpRequest()->getTag());
    //CCLog("response code: %d", statusCode);
    
    if (!response->isSucceed())
    {
        //CCLog("response failed");
        //CCLog("error buffer: %s", response->getErrorBuffer());
        result = SO_API_FAIL;
    }
    else
    {
        // dump data and convert to string
        std::vector<char> *buffer = response->getResponseData();
        std::string s(buffer->begin(), buffer->end());
        tmpArray = this->processJSON(s.c_str(), response->getHttpRequest()->getTag(), result);        
    }
    this->_gameInProgress = false;
    
    // Callback
    if (ScoreoidDelegate* delegate = Scoreoid::GetInstance()->getDelegate()) {
        return delegate->gameCallback(tmpArray,response->getHttpRequest()->getTag(),result);
    }    
}
void Scoreoid::HttpRequestLoginCallback(cocos2d::CCNode *sender, void *data)
{
    CCDictionary* tmpArray = NULL;
    int result = SO_API_FAIL;
    CCHttpResponse *response = (CCHttpResponse*)data;
    
    if (!response)
    {
        return;
    }
    //    if (0 != strlen(response->getHttpRequest()->getTag()))
    //    {
    //        //CCLog("%s completed", response->getHttpRequest()->getTag());
    //    }
    
    int statusCode = response->getResponseCode();
    char statusString[64] = {};
    sprintf(statusString, "HTTP Status Code: %d, tag = %s", statusCode, response->getHttpRequest()->getTag());
    
    if (!response->isSucceed())
    {
        result = SO_API_FAIL;
    }
    else
    {
        // dump data and convert to string
        std::vector<char> *buffer = response->getResponseData();
        std::string s(buffer->begin(), buffer->end());
        tmpArray = this->processJSON(s.c_str(), response->getHttpRequest()->getTag(), result);
    }
    
    if (this->loginStatus == GETPLAYER)
    {
        if (result == SO_API_SUCCES) // Login success
        {
            CCLog("API: LOGIN SUCCESS");
            this->loginStatus = IDLE;
            this->_scoreoidAvailable = true;
            if (ScoreoidDelegate* delegate = Scoreoid::GetInstance()->getDelegate()) {
            return delegate->scoreoidAvailable(tmpArray,response->getHttpRequest()->getTag(),result);
            }
        }
        else if (this->_shouldCreateNewPlayer)// Player unknown, create new one
        {
            CCLog("API: CREATE PLAYER");
            this->loginStatus = CREATEPLAYER;
            CCString* result = CCString::createWithFormat("username=%s",this->_localPlayerID);
            this->HttpRequest("http://www.scoreoid.com/api/createPlayer", result->getCString(),"createPlayer",callfuncND_selector(Scoreoid::HttpRequestLoginCallback));
        }
        else // login failed
        {
            CCLog("API: CREATE PLAYER FAILED");
            this->loginStatus = IDLE;
            this->_scoreoidAvailable = false;
            return;
        }
    }
    else if (this->loginStatus == CREATEPLAYER) // player is created
    {
        if (result == SO_API_SUCCES) // Player created, request player details
        {
            CCLog("API: CREATE PLAYER SUCCESS");
            this->loginStatus = PLAYERCREATED;
            CCString* result = CCString::createWithFormat("username=%s",this->_localPlayerID);
            this->HttpRequest("http://www.scoreoid.com/api/getPlayer", result->getCString(),"getPlayer",callfuncND_selector(Scoreoid::HttpRequestLoginCallback));
            
        }
        else // login failed
        {
            CCLog("API: LOGIN FAILED");
            this->loginStatus = IDLE;
            this->_scoreoidAvailable = false;
            return;            
        }
    }
    else if (this->loginStatus == PLAYERCREATED)
    {
        if (result == SO_API_SUCCES) // Player created, request player details
        {
            CCLog("API: PAYER CREATED AND LOGIN SUCCESS");
            this->loginStatus = IDLE;
            this->_scoreoidAvailable = true;
            if (ScoreoidDelegate* delegate = Scoreoid::GetInstance()->getDelegate()) {
                return delegate->scoreoidAvailable(tmpArray,response->getHttpRequest()->getTag(),result);
            }
        }
        else // login failed
        {
            CCLog("API: LOGIN FAILED");
            this->loginStatus = IDLE;
            this->_scoreoidAvailable = false;
            return;
        }        
    }
    // Callback
    //if (ScoreoidDelegate* delegate = Scoreoid::GetInstance()->getDelegate()) {
    //    return delegate->playerCallback(tmpArray,response->getHttpRequest()->getTag(),result);
    //}
}
std::string Scoreoid::handleHttpResult(CCHttpResponse* response)
{   
//    if (0 != strlen(response->getHttpRequest()->getTag()))
//    {
//        //CCLog("%s completed", response->getHttpRequest()->getTag());
//    }
    
    int statusCode = response->getResponseCode();
    char statusString[64] = {};
    sprintf(statusString, "HTTP Status Code: %d, tag = %s", statusCode, response->getHttpRequest()->getTag());
    //CCLog("response code: %d", statusCode);
    
//    if (!response->isSucceed())
//    {
//        //CCLog("response failed");
//        //CCLog("error buffer: %s", response->getErrorBuffer());
//    }
    // dump data and convert to string
    std::vector<char> *buffer = response->getResponseData();
    std::string s(buffer->begin(), buffer->end());
    return s;
}

/*
 Gets notifications for this game
 */
bool Scoreoid::getNotification() //method lets you pull your game’s in game notifications.
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        this->_gameInProgress = true;
        this->HttpRequest("http://www.scoreoid.com/api/getNotification", "","getNotification",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;
}

/*
 gets the total for the following game field’s bonus, gold, money, kills, lifes, time_played and unlocked_levels.
 field => String Value, (bonus, gold, money, kills, lifes, time_played, unlocked_levels) [Required]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 */
bool Scoreoid::getGameTotal(const char* field, const char* start_date, const char* end_date, const char* platform)
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        
        this->_gameInProgress = true;
        CCString* result = CCString::createWithFormat("field=%s&start_date=%s&end_date=%s&platform=%s",field,start_date,end_date,platform);
        this->HttpRequest("http://www.scoreoid.com/api/getGameTotal", result->getCString(),"getGameTotal",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 gets the lowest value for the following game field’s bonus, gold, money, kills, lifes, time_played and unlocked_levels.
 field => String Value, (bonus,best_score, gold, money, kills, lifes, time_played, unlocked_levels) [Required]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 */
bool Scoreoid::getGameLowest(const char* field, const char* start_date, const char* end_date, const char* platform)
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        this->_gameInProgress = true;
        CCString* result = CCString::createWithFormat("field=%s&start_date=%s&end_date=%s&platform=%s",field,start_date,end_date,platform);
        this->HttpRequest("http://www.scoreoid.com/api/getGameLowest", result->getCString(),"getGameLowest",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 gets the top value for the following game field’s bonus, gold, money, kills, lifes, time_played and unlocked_levels.
 field => String Value, (bonus, best_score, gold, money, kills, lifes, time_played, unlocked_levels) [Required]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 */
bool Scoreoid::getGameTop(const char* field, const char* start_date, const char* end_date, const char* platform)
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        this->_gameInProgress = true;
        CCString* result = CCString::createWithFormat("field=%s&start_date=%s&end_date=%s&platform=%s",field,start_date,end_date,platform);
        this->HttpRequest("http://www.scoreoid.com/api/getGameTop", result->getCString(),"getGameTop",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 gets the average for the following game field’s bonus, gold, money, kills, lifes, time_played and unlocked_levels.
 field => String Value, (bonus, best_score, gold, money, kills, lifes, time_played, unlocked_levels) [Required]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 */
bool Scoreoid::getGameAverage(const char* field, const char* start_date, const char* end_date, const char* platform)
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        this->_gameInProgress = true;
        CCString* result = CCString::createWithFormat("field=%s&start_date=%s&end_date=%s&platform=%s",field,start_date,end_date,platform);
        this->HttpRequest("http://www.scoreoid.com/api/getGameAverage", result->getCString(),"getGameAverage",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 API method lets you get all the players for a specif game.
 order_by => String Value, (date or score) [Optional]
 order => String Value, asc or desc [Optional]
 limit => Number Value, the limit, "20" retrieves rows 1 - 20 | "10,20" retrieves 20 scores starting from the 10th [Optional]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => String Value, needs to match the string value that was used when creating the player [Optional]
 */
bool Scoreoid::getPlayers(const char* order_by, const char* order, const char* limit, const char* start_date, const char* end_date, const char* platform)
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        this->_gameInProgress = true;
        CCString* result = CCString::createWithFormat("order_by=%s&order=%s&limit=%s&start_date=%s&end_date=%s&platform=%s",order_by,order,limit,start_date,end_date,platform);
        this->HttpRequest("http://www.scoreoid.com/api/getPlayers", result->getCString(),"getPlayers",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you pull a specific field from your game info.
 field => String Value, (name, short_description, description, game_type, version, levels, platform, play_url, website_url,
 created, updated, player_count, scores_count, locked, status) [Required]
 */
bool Scoreoid::getGameField(const char* field)
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        this->_gameInProgress = true;
        CCString* result = CCString::createWithFormat("field=%s",field);
        this->HttpRequest("http://www.scoreoid.com/api/getGameField", result->getCString(),"getGameField",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you pull all your game information.
 */
bool Scoreoid::getGame()
{
    if (this->_scoreoidAvailable && !this->_gameInProgress)
    {
        this->_gameInProgress = true;
        this->HttpRequest("http://www.scoreoid.com/api/getGame", "","getGame",callfuncND_selector(Scoreoid::HttpRequestGameCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you pull a specific field from your player info.
 username => String Value, the players username [Required]
 field => String Value, the selected field (username, password, unique_id, first_name, last_name, email, platform, created,
 updated, bonus, achievements, best_score, gold, money, kills, lives, time_played, unlocked_levels, unlocked_items, inventory,
 last_level, current_level, current_time, current_bonus, current_kills, current_achievements, current_gold, current_unlocked_levels,
 current_unlocked_items, current_lifes, xp, energy, boost, latitude, longitude, game_state, platform, rank) [Required]
 */
bool Scoreoid::getPlayerField(const char* field)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("field=%s",field);
        this->HttpRequest("http://www.scoreoid.com/api/getPlayerField", result->getCString(),"getPlayerField",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;
}

/*
 method lets you pull all the scores for a player.
 username => String Value, the players username [Required]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 difficulty => Integer Value, between 1 to 10 (don't use 0 as it's the default value) [Optional]
 */
bool Scoreoid::getPlayerScores(const char* username, const char* start_date, const char* end_date, const char* difficulty)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("username=%s&start_date=%s&end_date=%s&difficulty=%s",username,start_date,end_date,difficulty);
        this->HttpRequest("http://www.scoreoid.com/api/getPlayerScores", result->getCString(),"getPlayerScores",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method deletes a player and all the players scores.
 username => String Value, the players username [Required]
 */
bool Scoreoid::deletePlayer(const char* username)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("username=%s",username);
        this->HttpRequest("http://www.scoreoid.com/api/deletePlayer", result->getCString(),"deletePlayer",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method Check if player exists and returns the player information. Post parameters work as query conditions. This method can be used for login by adding username and password parameters.
 id =>  The players ID [String] [Optional]
 username => String Value, the players username [Required]
 password => The players password [String] [Optional]
 email => The players email [String] [Optional]
 */
bool Scoreoid::getPlayer(const char* username, const char* id, const char* password, const char* email)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("username=%s&id=%s&password=%s&email=%s",username,id,password,email);
        this->HttpRequest("http://www.scoreoid.com/api/getPlayer", result->getCString(),"getPlayer",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you edit your player information.
 username => String Value, the players username [Required]
 fields = string with all fields in format password=[value] separated by &
 password, unique_id, first_name, last_name, email, created, updated, bonus, achievements, gold, money, kills, lives, time_played, unlocked_levels, unlocked_items, inventory, last_level, current_level, current_time, current_bonus, current_kills, current_achievements, current_gold, current_unlocked_levels, current_unlocked_items, current_lifes, xp, energy, boost, latitude, longitude, game_state, platform)
 For description of all the fields www.scoreoid.com
 */
bool Scoreoid::editPlayer(const char* username, const char* fields)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("username=%s&fields=%s",username,fields);
        this->HttpRequest("http://www.scoreoid.com/api/editPlayer", result->getCString(),"editPlayer",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you count all your players.
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 */
bool Scoreoid::countPlayers(const char* start_date, const char*end_date, const char* platform)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("start_date=%s&end_date=%s&platform=%s",start_date,end_date,platform);
        this->HttpRequest("http://www.scoreoid.com/api/countPlayers", result->getCString(),"countPlayers",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you update your player field’s.
 username => String Value, the players username [Required]
 field => String Value, (password, unique_id, first_name, last_name, email, created, updated, bonus, achievements, gold, money, kills, lives, time_played, unlocked_levels, unlocked_items, inventory, last_level, current_level, current_time, current_bonus, current_kills, current_achievements, current_gold, current_unlocked_levels, current_unlocked_items, current_lifes, xp, energy, boost, latitude, longitude, game_state, platform) [Required]
 value => String Value, the field value [Required]
 */
bool Scoreoid::updatePlayerField(const char* username, const char* field, const char* value)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("username=%s&field=%s&value=%s",username,field,value);
        this->HttpRequest("http://www.scoreoid.com/api/updatePlayerField", result->getCString(),"updatePlayerField",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you create a player with a number of optional values.
 username => The players username [String] [Required]
 fields = string with all fields in format password=[value] separated by &
 password, unique_id, first_name, last_name, email, created, updated, bonus, achievements, gold, money, kills, lives, time_played, unlocked_levels, unlocked_items, inventory, last_level, current_level, current_time, current_bonus, current_kills, current_achievements, current_gold, current_unlocked_levels, current_unlocked_items, current_lifes, xp, energy, boost, latitude, longitude, game_state, platform)
 */
bool Scoreoid::createPlayer(const char* username, const char* fields)
{
    if (this->_scoreoidAvailable && !this->_playerInProgress)
    {
        this->_playerInProgress = true;
        CCString* result = CCString::createWithFormat("username=%s&fields",username,fields);
        this->HttpRequest("http://www.scoreoid.com/api/createPlayer", result->getCString(),"createPlayer",callfuncND_selector(Scoreoid::HttpRequestPlayerCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you count all your game best scores.
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 difficulty => Integer Value, between 1 to 10 (don't use 0 as it's the default value) [Optional]
 */
bool Scoreoid::countBestScores(const char* start_date, const char*end_date, const char* platform, const char* difficulty)
{
    if (this->_scoreoidAvailable && !this->_scoreInProgress)
    {
        this->_scoreInProgress = true;
        CCString* result = CCString::createWithFormat("start_date=%s&end_date=%s&platform=%s&difficulty=%s",start_date,end_date,platform,difficulty);
        this->HttpRequest("http://www.scoreoid.com/api/countBestScores", result->getCString(),"countBestScores",callfuncND_selector(Scoreoid::HttpRequestScoreCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you get all your game average scores.
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 difficulty => Integer Value, between 1 to 10 (don't use 0 as it's the default value) [Optional]
 */
bool Scoreoid::getAverageScore(const char* start_date, const char*end_date, const char* platform, const char* difficulty)
{
    if (this->_scoreoidAvailable && !this->_scoreInProgress)
    {
        this->_scoreInProgress = true;
        CCString* result = CCString::createWithFormat("start_date=%s&end_date=%s&platform=%s&difficulty=%s",start_date,end_date,platform,difficulty);
        this->HttpRequest("http://www.scoreoid.com/api/getAverageScore", result->getCString(),"getAverageScore",callfuncND_selector(Scoreoid::HttpRequestScoreCallback));
    }
    else
    {
        return false;
    }
    return true;
}

/*
 method lets you get all your games best scores.
 order_by => String Value, (date or score) [Optional]
 order => String Value, asc or desc [Optional]
 limit => Number Value, the limit, "20" retrieves rows 1 - 20 | "10,20" retrieves 20 scores starting from the 10th [Optional]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 difficulty => Integer Value, between 1 to 10 (don't use 0 as it's the default value) [Optional]     */
bool Scoreoid::getBestScores(const char* order_by, const char* order, const char* limit,const char* start_date, const char*end_date, const char* platform, const char* difficulty)
{
    if (this->_scoreoidAvailable && !this->_scoreInProgress)
    {
        this->_scoreInProgress = true;
        CCString* result = CCString::createWithFormat("order_by=%s&order=%s&limit=%s&start_date=%s&end_date=%s&platform=%s&difficulty=%s",order_by,order,limit,start_date,end_date,platform,difficulty);
        this->HttpRequest("http://www.scoreoid.com/api/getBestScores", result->getCString(),"getBestScores",callfuncND_selector(Scoreoid::HttpRequestScoreCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you count all your game scores.
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 difficulty => Integer Value, between 1 to 10 (don't use 0 as it's the default value) [Optional]
 */
bool Scoreoid::countScores(const char* start_date, const char*end_date, const char* platform, const char* difficulty)
{
    if (this->_scoreoidAvailable && !this->_scoreInProgress)
    {
        this->_scoreInProgress = true;
        CCString* result = CCString::createWithFormat("start_date=%s&end_date=%s&platform=%s&difficulty=%s",start_date,end_date,platform,difficulty);
        this->HttpRequest("http://www.scoreoid.com/api/countScores", result->getCString(),"countScores",callfuncND_selector(Scoreoid::HttpRequestScoreCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you pull all your game scores.
 order_by => String Value, (date or score) [Optional]
 order => String Value, asc or desc [Optional]
 limit => Number Value, the limit, "20" retrieves rows 1 - 20 | "10,20" retrieves 20 scores starting from the 10th [Optional]
 start_date => String Value, YYY-MM-DD format [Optional]
 end_date => String Value, YYY-MM-DD format [Optional]
 platform => The players platform needs to match the string value that was used when creating the player  [Optional]
 difficulty => Integer Value, between 1 to 10 (don't use 0 as it's the default value) [Optional]
 */
bool Scoreoid::getScores(const char* order_by, const char* order, const char* limit,const char* start_date, const char*end_date, const char* platform, const char* difficulty)
{
    if (this->_scoreoidAvailable && !this->_scoreInProgress)
    {
        this->_scoreInProgress = true;
        CCString* result = CCString::createWithFormat("order_by=%s&order=%s&limit=%s&start_date=%s&end_date=%s&platform=%s&difficulty=%s",order_by,order,limit,start_date,end_date,platform,difficulty);
        this->HttpRequest("http://www.scoreoid.com/api/getScores", result->getCString(),"getScores",callfuncND_selector(Scoreoid::HttpRequestScoreCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 method lets you create a user score.
 username => String Value, if user does not exist it well be created [Required]
 score => Integer Value, [Required]
 platform => String Value, [Optional]
 unique_id => Integer Value, [Optional]
 difficulty => Integer Value, between 1 to 10 (don't use 0 as it's the default value) [Optional]
 */
bool Scoreoid::createScore(const char* username, const char* score, const char* platform, const char* unique_id, const char* difficulty)
{
    if (this->_scoreoidAvailable && !this->_scoreInProgress)
    {
        this->_scoreInProgress = true;
        CCString* result = CCString::createWithFormat("username=%s&score=%s&platform=%s&unique_id=%s&difficulty=%s",username,score,platform,unique_id,difficulty);
        this->HttpRequest("http://www.scoreoid.com/api/createScore", result->getCString(),"createScore",callfuncND_selector(Scoreoid::HttpRequestScoreCallback));
    }
    else
    {
        return false;
    }
    return true;    
}

/*
 * Login the player or create a new one
 */
bool Scoreoid::login(const char *uniqueID, bool createPlayer)
{
    this->_scoreoidAvailable = false;
    if (this->loginStatus != IDLE ||strlen(uniqueID) == 0 )
    {
        return false;
    }
    this->_shouldCreateNewPlayer = createPlayer;
    this->loginStatus = GETPLAYER;
    this->_localPlayerID = uniqueID;
    CCString* result = CCString::createWithFormat("username=%s",uniqueID);
    this->HttpRequest("http://www.scoreoid.com/api/getPlayer", result->getCString(),"getPlayer",callfuncND_selector(Scoreoid::HttpRequestLoginCallback));

    return true;
}