/*
Copyright (C) 2014 - 2015 Eaton

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*!
 *  \file thresholdrulesimple.h
 *  \author Alena Chernikava <AlenaChernikava@Eaton.com>
 *  \brief Simple threshold rule representation
 */
#ifndef SRC_THRESHOLDRULESIMPLE_H
#define SRC_THRESHOLDRULESIMPLE_H

#include "rule.h"
#include <cxxtools/jsondeserializer.h>

class ThresholdRuleSimple : public Rule
{
public:

    ThresholdRuleSimple(){};

    // throws -> it is pattern but with errors
    // 0 - ok
    // 1 - it is not pattern rule
    // TODO json string is bad idea, redo to serialization in future
    int fill(cxxtools::JsonDeserializer &json, const std::string &json_string)
    {
        const cxxtools::SerializationInfo *si = json.si();
        if ( si->findMember("threshold") == NULL ) {
            return 1;
        }
        auto threshold = si->getMember("threshold");
        if ( threshold.category () != cxxtools::SerializationInfo::Object ) {
            zsys_info ("Root of json must be an object with property 'threshold'.");
            throw std::runtime_error("Root of json must be an object with property 'threshold'.");
        }

        // target
        auto target = threshold.getMember("target");
        if ( target.category () != cxxtools::SerializationInfo::Value ) {
            return 1;
        }
        zsys_info ("it is simple threshold rule");

        target >>= _metric;
        _json_representation = json_string;
        threshold.getMember("rule_name") >>= _name;
        threshold.getMember("element") >>= _element;
        // values
        // TODO check low_critical < low_warnong < high_warning < hign crtical
        std::map<std::string,double> tmp_values;
        auto values = threshold.getMember("values");
        if ( values.category () != cxxtools::SerializationInfo::Array ) {
            zsys_info ("parameter 'values' in json must be an array.");
            throw std::runtime_error("parameter 'values' in json must be an array");
        }
        values >>= tmp_values;
        globalVariables(tmp_values);

        // outcomes
        auto outcomes = threshold.getMember("results");
        if ( outcomes.category () != cxxtools::SerializationInfo::Array ) {
            zsys_info ("parameter 'results' in json must be an array.");
            throw std::runtime_error ("parameter 'results' in json must be an array.");
        }
        outcomes >>= _outcomes;
        return 0;
    }

    int evaluate (const MetricList &metricList, PureAlert **pureAlert) {
        // ASSUMPTION: constants are in values
        //  high_critical
        //  high_warning
        //  low_warning
        //  low_critical
        const auto GV = getGlobalVariables();
        auto valueToCheck = GV.find ("high_critical");
        if ( valueToCheck != GV.cend() ) {
            if ( valueToCheck->second < metricList.getLastMetric().getValue() ) {
                auto outcome = _outcomes.find ("high_critical");
                *pureAlert = new PureAlert(ALERT_START, metricList.getLastMetric().getTimestamp() , outcome->second._description, this->_element);
                (*pureAlert)->_severity = outcome->second._severity;
                (*pureAlert)->_actions = outcome->second._actions;
                return 0;
            }
        }
        valueToCheck = GV.find ("high_warning");
        if ( valueToCheck != GV.cend() ) {
            if ( valueToCheck->second < metricList.getLastMetric().getValue() ) {
                auto outcome = _outcomes.find ("high_warning");
                *pureAlert = new PureAlert(ALERT_START, metricList.getLastMetric().getTimestamp() , outcome->second._description, this->_element);
                (*pureAlert)->_severity = outcome->second._severity;
                (*pureAlert)->_actions = outcome->second._actions;
                return 0;
            }
        }
        valueToCheck = GV.find ("low_critical");
        if ( valueToCheck != GV.cend() ) {
            if ( valueToCheck->second > metricList.getLastMetric().getValue() ) {
                auto outcome = _outcomes.find ("low_critical");
                *pureAlert = new PureAlert(ALERT_START, metricList.getLastMetric().getTimestamp() , outcome->second._description, this->_element);
                (*pureAlert)->_severity = outcome->second._severity;
                (*pureAlert)->_actions = outcome->second._actions;
                return 0;
            }
        }
        valueToCheck = GV.find ("low_warning");
        if ( valueToCheck != GV.cend() ) {
            if ( valueToCheck->second > metricList.getLastMetric().getValue() ) {
                auto outcome = _outcomes.find ("low_warning");
                *pureAlert = new PureAlert(ALERT_START, metricList.getLastMetric().getTimestamp() , outcome->second._description, this->_element);
                (*pureAlert)->_severity = outcome->second._severity;
                (*pureAlert)->_actions = outcome->second._actions;
                return 0;
            }
        }
        // if we are here -> no alert was detected
        // TODO actions
        *pureAlert = new PureAlert(ALERT_RESOLVED, metricList.getLastMetric().getTimestamp(), "ok", this->_element);
        (**pureAlert).print();
        return 0;
    };

    bool isTopicInteresting(const std::string &topic) const {
        return ( _metric == topic ? true : false );
    };

    friend int readRule (std::istream &f, Rule **rule);

    std::vector<std::string> getNeededTopics(void) const {
        return {_metric};
    };

private:
    // needed metric topic
    std::string _metric;
};


#endif // SRC_THRESHOLDRULESIMPLE_H
