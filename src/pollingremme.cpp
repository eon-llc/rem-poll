#include <pollingremme.hpp>
#include <rem.system/rem.system.hpp>

void pollingremme::create(name user, string subject, option_list options, bool is_token_poll, bool producers_only, bool guardians_only, time_point expires_at) {

  require_auth(user);

  check(!subject.empty(), "Poll subject can not be blank.");
  check(options.size() > 1 && options.size() < 11, "A poll must have between 2 and 10 choices.");

  if(expires_at.sec_since_epoch() != 0) {
    check(expires_at.sec_since_epoch() > current_time_point().sec_since_epoch(), "Poll expiration must be in the future.");
  }

  polls_table polls(get_self(), get_self().value);

  polls.emplace(user, [&](auto& p) {
    uint64_t id = polls.available_primary_key();
    time_point ct = current_time_point();

    p.id = id;
    p.user = user;
    p.subject = subject;
    p.options = options;
    p.is_token_poll = is_token_poll;
    p.producers_only = producers_only;
    p.guardians_only = guardians_only;
    p.created_at = ct;
    p.expires_at = expires_at;
  });
}

void pollingremme::vote(name user, uint64_t poll_id, uint8_t option_id) {

  require_auth(user);

  // check if poll exists and isn't expired
  polls_table polls(get_self(), get_self().value);
  const poll_t & p = polls.get(poll_id, "Poll with this id does not exist.");
  check(p.expires_at.sec_since_epoch() > current_time_point().sec_since_epoch(), "Voting period for this poll has ended.");

  // check if voter qualifies for this poll
  if(p.producers_only) {

  }

  if(p.guardians_only) {

  }

  // check if this voter can vote
  votes_table votes(get_self(), get_self().value);
  check(option_id < p.options.size(), "Option with this id does not exist in this poll.");
  check(votes.find(p.id) == votes.end(), "This account has already voted in this poll.");

  if (p.is_token_poll) {

    //token token("REM");
    //print(token);
    //asset balance = token.get_balance(voter, p.token.name());
    //eosio_assert(balance.is_valid(), "Invalid token balance.");
    //eosio_assert(balance.amount > 0, "Voter must have a stake of more than 0 tokens.");
    //double stake = balance.amount / std::pow(10, stake.symbol.precision());
    //print(stake);
  }

  votes.emplace(user, [&](auto& v) {
    time_point ct = current_time_point();

    v.poll_id = poll_id;
    v.user = user;
    v.option_id = option_id;
    v.created_at = ct;
  });
}

bool pollingremme::is_producer( const name& user ) const {
  eosiosystem::producers_table _producers_table( system_account, system_account.value );
  return _producers_table.find( user.value ) != _producers_table.end();
}

bool pollingremme::is_active_producer( const name& user ) const {
  eosiosystem::producers_table _producers_table( system_account, system_account.value );
  return _producers_table.find( user.value ) != _producers_table.end();
}

bool pollingremme::is_voter( const name& user ) const {
  eosiosystem::producers_table _producers_table( system_account, system_account.value );
  return _producers_table.find( user.value ) != _producers_table.end();
}

EOSIO_DISPATCH(pollingremme, (create)(vote))