#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("pollingremme")]] pollingremme : public eosio::contract {

  public:

    pollingremme(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {}
    typedef vector<string> option_list;

    [[eosio::action]]
    void create(name user, string subject, option_list options, bool is_token_poll, bool producers_only, bool guardians_only, time_point open_until);

    [[eosio::action]]
    void vote(name user, uint64_t poll_id, uint8_t option_id);

  private:

    static constexpr name system_account = name("rem");

    struct [[eosio::table]] poll_t {
      uint64_t        id;
      name            user;
      string          subject;
      option_list     options;
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

    typedef multi_index<name("polls"), poll_t> polls_table;
    typedef multi_index<name("votes"), vote_t> votes_table;

    bool is_producer( const name& user ) const;
    bool is_active_producer( const name& user ) const;
    bool is_voter( const name& user ) const;
};
