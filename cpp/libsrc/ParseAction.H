//
//  ParseAction                                                            
//  -----------                                                            
//                                                                         
//  This is the unencoded version of a parse action. It's not terribly big 
//  but it's much bigger than the encoded version. Unfortunately we need   
//  to use these as map keys so we have to support some comparison         
//  operators.                                                             
//

#ifndef PARSE_ACTION_H
#define PARSE_ACTION_H

#include <cstdint>

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

enum ParseActionType : int
{
    ActionLaShift = 0,
    ActionShift = 1,
    ActionReduce = 2,
    ActionGoto = 3,
    ActionRestart = 4,
    ActionAccept = 5,
    ActionError = 6
};

struct ParseAction
{

    ParseActionType action_type = ParseActionType::ActionError;
    int64_t goto_state = -1;
    int64_t rule_num = -1;
    int64_t fallback_state = -1;

    bool operator<(const ParseAction& rhs) const
    {
        if (action_type < rhs.action_type) return true;
        if (action_type > rhs.action_type) return false;
        if (goto_state < rhs.goto_state) return true;
        if (goto_state > rhs.goto_state) return false;
        if (rule_num < rhs.rule_num) return true;
        if (rule_num > rhs.rule_num) return false;
        return fallback_state < rhs.fallback_state;
    }

    bool operator>(const ParseAction& rhs) const
    {
        return rhs < *this;
    } 

    bool operator==(const ParseAction& rhs) const
    {
        return action_type == rhs.action_type &&
               goto_state == rhs.goto_state &&
               rule_num == rhs.rule_num &&
               fallback_state == rhs.fallback_state;
    }

    bool operator!=(const ParseAction& rhs) const
    {
        if (*this == rhs)
        {
            return false;  
        }
        else
        {
            return true;
        }
    }

};

} // namespace hoshi

#endif // PARSE_ACTION_H
