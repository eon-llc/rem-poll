#include <pollingremme.hpp>
#include <rem.system/rem.system.hpp>

void pollingremme::create(name user, string subject, string description, option_list options, bool is_token_poll, bool producers_only, bool guardians_only, time_point expires_at) {

  require_auth(user);

  check(!subject.empty(), "Poll subject can not be blank.");
  check(subject.size() < 140, "Poll subject should be no longer than 140 characters.");
  check(options.size() > 1 && options.size() < 11, "A poll must have between 2 and 10 choices.");

  if(expires_at.sec_since_epoch() != 0) {
    check(expires_at.sec_since_epoch() > current_time_point().sec_since_epoch(), "Poll expiration must be in the future.");
  }

  polls_table polls(get_self(), get_self().value);

  polls.emplace(user, [&](auto& p) {
    uint64_t id = polls.available_primary_key();
    time_point ct = current_time_point();

    option_results results = {};
    results.resize(options.size());
    transform(options.begin(), options.end(), results.begin(), [&](string str) {
        check(!str.empty(), "Option names can't be blank");
        return option_result(str);
    });

    p.id = id;
    p.user = user;
    p.subject = subject;
    p.description = description;
    p.options = results;
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

  // check poll expiration if it's set
  if(p.expires_at.sec_since_epoch() != 0) {
    check(p.expires_at.sec_since_epoch() > current_time_point().sec_since_epoch(), "Voting period for this poll has ended.");
  }

  // check if voter qualifies for this poll
  if(p.producers_only) {
    check(is_producer(user), "This poll requires block producer authorization.");
  }

  if(p.guardians_only) {
    check(is_guardian(user), "This poll requires guardian authorization.");
  }

  // check if this voter can vote
  votes_table votes(get_self(), poll_id);
  check(option_id < p.options.size(), "Option with this id does not exist in this poll.");
  check(votes.find(user.value) == votes.end(), "This account has already voted in this poll.");

  uint64_t weight;

  if(p.is_token_poll) {
    weight = get_voter_stake(user);
  } else {
    weight = 1;
  }

  votes.emplace(user, [&](auto& v) {
    time_point ct = current_time_point();

    v.poll_id = poll_id;
    v.user = user;
    v.weight = weight;
    v.option_id = option_id;
    v.created_at = ct;
  });

  polls.modify(p, user, [&](auto& p) {
    p.options[option_id].votes += weight;
  });
}

void pollingremme::comment(name user, uint64_t poll_id, string message) {

  require_auth(user);

  // check if poll exists and isn't expired
  polls_table polls(get_self(), get_self().value);
  const poll_t & p = polls.get(poll_id, "Poll with this id does not exist.");

  comments_table comments(get_self(), poll_id);
  check(!message.empty(), "Message can't be blank.");

  // rate limit comments to 1 per 60 seconds
  const auto ct = current_time_point().sec_since_epoch();
  const auto rate_limit = 60;
  auto most_recent_comment = rate_limit + 1;
  bool has_prior_comment = false;

  for (auto iter = comments.begin(); iter != comments.end() && iter->user == user; iter++) {

    has_prior_comment = true;

    if(ct - iter->created_at.sec_since_epoch() < most_recent_comment) {
      most_recent_comment = ct - iter->created_at.sec_since_epoch();
    }
  }

  check(!has_prior_comment || (has_prior_comment && most_recent_comment > rate_limit), "Comments are limited to 1 for every " + to_string(rate_limit) + " seconds.");

  // save new comment
  uint64_t id = comments.available_primary_key();

  comments.emplace(user, [&](auto& c) {
    time_point ct = current_time_point();

    c.id = id;
    c.poll_id = poll_id;
    c.user = user;
    c.message = message;
    c.created_at = ct;
  });
}

uint64_t pollingremme::get_voter_stake( const name& user ) const {
    eosiosystem::voters_table _voters_table( system_account, system_account.value );
    auto voter = _voters_table.find( user.value );

    if(voter != _voters_table.end()) {
        return voter->locked_stake;
    } else {
        return 0;
    }
}

bool pollingremme::is_producer( const name& user ) const {
    eosiosystem::producers_table _producers_table( system_account, system_account.value );
    auto prod = _producers_table.find( user.value );
    return prod != _producers_table.end() && prod->is_active == 1;
}

bool pollingremme::is_guardian( const name& user ) const {
    eosiosystem::voters_table _voters_table( system_account, system_account.value );
    auto voter = _voters_table.find( user.value );
    bool exists = voter != _voters_table.end();
    bool enough_staked = voter->locked_stake >= guardian_stake_threshold;
    bool is_active = voter->last_reassertion_time.sec_since_epoch() > (current_time_point().sec_since_epoch() - 60*60*24*30);
    return exists && enough_staked && is_active;
}

void pollingremme::deletedata(int64_t poll_id) {
    require_auth(get_self());

    // delete polls
    polls_table polls(get_self(), get_self().value);
    auto p_itr = polls.begin();
    while(p_itr != polls.end()){
        p_itr = polls.erase(p_itr);
    }

    // delete votes
    votes_table votes(get_self(), poll_id);
    auto v_itr = votes.begin();
    while(v_itr != votes.end()){
        v_itr = votes.erase(v_itr);
    }

    // delete comments
    comments_table comments(get_self(), poll_id);
    auto c_itr = comments.begin();
    while(c_itr != comments.end()){
        c_itr = comments.erase(c_itr);
    }
}

EOSIO_DISPATCH(pollingremme, (create)(vote)(comment)(deletedata))