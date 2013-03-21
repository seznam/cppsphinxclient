
#include <sphinxclient/sphinxclient.h>
#include <stdio.h>

//------------------------------------------------------------------------------


int main()
{
    Sphinx::ConnectionConfig_t config (
        "127.0.0.1", // hostname
        8355,        // port
        true,        // keepalive
        2000,        // connect timeout
        20000,       // read timeout
        2000         // write timeout
    );

    Sphinx::Client_t connection(config);
    Sphinx::Response_t result;
    Sphinx::SearchConfig_t settings(Sphinx::VER_COMMAND_SEARCH_2_0_5);

    printf("starting.....\n");

    // search settings
    settings.setPaging(0, 20);
    settings.setMatchMode(Sphinx::SPH_MATCH_EXTENDED2);
    settings.setMaxMatches(1000);
    settings.setSearchedIndexes("idx_product_pairing");
    settings.setQueryComment("comment");
    settings.setMaxQueryTime(10000);


    // sorting - relevance ascending
    settings.setSorting(Sphinx::SPH_SORT_RELEVANCE);

    printf("searching.....\n");
    try{
        //single query
        connection.query("", settings, result);
        printf("query success.\n");
    }
    catch(Sphinx::Error_t e)
    {
        printf("query error:\n%s\n", e.errMsg.c_str());
        return 2;
    }

    printf("field count:       %d\n", result.field.size());
    printf("attribute count:   %d\n", result.attribute.size());
    printf("match count:       %d\n", result.entry.size());
    printf("word count:        %d\n", result.word.size());

    printf("\nFields:\n");
    for (int i=0;i<result.field.size();i++)
    {
        printf("    %s\n", result.field[i].c_str());
    }//for

    printf("\nAttributes:\n");
    for (Sphinx::AttributeTypes_t::iterator it=result.attribute.begin() ;
         it!=result.attribute.end();it++)
    {
        if (it->second & Sphinx::SPH_ATTR_MULTI) {
            printf("    name: %s, multi type: %d\n", it->first.c_str(), (it->second ^ Sphinx::SPH_ATTR_MULTI));
        }//if
        else
            printf("    name: %s, type: %d\n", it->first.c_str(), it->second);
    }//for

    printf("\nWords:\n");
    for (std::map<std::string, Sphinx::WordStatistics_t>::iterator it
         = result.word.begin() ; it!=result.word.end() ; it++)
    {
        printf("    Word %s: %d docs / %d hits\n", it->first.c_str(), it->second.docsHit, it->second.totalHits);
    }//for

    printf("entries: %d\n", result.entriesGot);
    printf("documents: %d\n", result.entriesFound);
    printf("duration: %d\n", result.timeConsumed);
    printf("64bit ID: %d\n", result.use64bitId);

    // print results with attributes
    printf("\nResponse:\n");

    int i = 0; 
    for (std::vector<Sphinx::ResponseEntry_t>::const_iterator it = result.entry.begin();
         it != result.entry.end(); it++) {

        i++;
        printf("%d) id: %ld ", i, it->documentId);
        for (std::map<std::string, Sphinx::Value_t>::const_iterator j =
                            it->attribute.begin();
                j != it->attribute.end() ; j++)
        {
            switch(j->second.getValueType()) {
            case Sphinx::VALUETYPE_FLOAT:
                printf("%s:%.2f | ", j->first.c_str(), (float)(j->second));
                break;
            case Sphinx::VALUETYPE_VECTOR: {
                char comma[2] = {'\0', '\0'};
                std::vector<Sphinx::Value_t> vec = j->second;
                printf("%s:(", j->first.c_str());
                for (std::vector<Sphinx::Value_t>::iterator vi=vec.begin() ; vi!=vec.end() ; vi++) {
                    if(vi->getValueType() == Sphinx::VALUETYPE_FLOAT)
                        printf("%s%ff", comma, (float)(*vi));
                    else if (vi->getValueType() == Sphinx::VALUETYPE_UINT64)
                        printf("%s%lu", comma, (uint64_t)(*vi));
                    else
                        printf("%s%dd", comma, (uint32_t)(*vi));
                    comma[0] = ',';
                }//for
                printf(") | ");
                break; }
            case Sphinx::VALUETYPE_UINT64:
                printf("%s:%lu | ", j->first.c_str(), (uint64_t)(j->second));
                break;
            case Sphinx::VALUETYPE_STRING:
                printf("%s:<%s> | ", j->first.c_str(), ((std::string)(j->second)).c_str());
                break;
            case Sphinx::VALUETYPE_UINT32:
            default:
                printf("%s:%d | ", j->first.c_str(), (uint32_t)(j->second));
                break;
            }//switch
        }
        printf("\n");
    }//for response entries

    printf("----------------------------- end ------------------------\n");

    return 0;
}//main

