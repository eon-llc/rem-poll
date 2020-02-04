#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("pollingremme")]] pollingremme : public eosio::contract {

  public:

    pollingremme(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {}

    struct option {
        string name;
        option(string name) : name(name) {}
        option() {}
        EOSLIB_SERIALIZE(option, (name))
    };

    struct option_result : option {
        uint64_t votes = 0;
        option_result(const string& name, uint64_t votes) : option(name), votes(votes) {}
        option_result(const string& name) : option_result(name, 0) {}
        option_result() {}
        EOSLIB_SERIALIZE(option_result, (name)(votes))
    };

    typedef vector<string> option_list;
    typedef vector<option_result> option_results;

    [[eosio::action]]
    void create(name user, string subject, string description, option_list options, bool is_token_poll, bool producers_only, bool guardians_only, time_point expires_at);

    [[eosio::action]]
    void vote(name user, uint64_t poll_id, uint8_t option_id);

    [[eosio::action]]
    void comment(name user, uint64_t poll_id, string message);

    [[eosio::action]]
    void deletedata();

  private:

    static const uint32_t guardian_stake_threshold = 2500000000;

    static constexpr name system_account = name("rem");

    struct [[eosio::table]] poll_t {
        uint64_t        id;
        name            user;
        string          subject;
        string          description;
        option_results  options;
        bool            is_token_poll;
        bool            producers_only;
        bool            guardians_only;
        time_point      created_at;
        time_point      expires_at;

        uint64_t        primary_key() const { return id; }
        uint64_t        get_reverse_key() const { return ~id; }
    };

    struct [[eosio::table]] vote_t {
        uint64_t        poll_id;
        name            user;
        uint8_t         option_id;
        time_point      created_at;

        uint64_t        primary_key() const { return poll_id; }
        uint64_t        get_reverse_key() const { return ~poll_id; }
    };

    struct [[eosio::table]] comment_t {
        uint64_t        poll_id;
        name            user;
        string          message;
        time_point      created_at;

        uint64_t        primary_key() const { return poll_id; }
        uint64_t        get_reverse_key() const { return ~poll_id; }
    };

    typedef multi_index<name("polls"), poll_t> polls_table;
    typedef multi_index<name("votes"), vote_t> votes_table;
    typedef multi_index<name("comments"), comment_t> comments_table;

    uint32_t get_voter_stake( const name& user ) const;
    bool is_producer( const name& user ) const;
    bool is_guardian( const name& user ) const;
};
