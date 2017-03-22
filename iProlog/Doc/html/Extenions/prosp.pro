%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% A simple implementation of Prospector's probabilistic reasoning
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

op(850, xfx, ==>)!

odds(P) is P / (1 - P).
prob(O) is O / (1 + O).

posterior(Consequent) is prob(M * odds(prior(Consequent))) :-
	Antecedent ==> Consequent(LS, LN),
	M is odds_multiplier(prior(Antecedent), posterior(Antecedent), LS, LN).

odds_multiplier(Prior, Posterior, LS, LN) is LS * (Posterior - Prior) / (1 - Prior) :-
	Prior < Posterior.
odds_multiplier(Prior, Posterior, LS, LN) is LN * (Prior - Posterior) / Prior :-
	Prior > Posterior.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% These are the rules for infering a hypothesis given the
% presence of absence of some other evidence.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

rcib ==> smir(20, 1).
rcvp ==> smir(4, 1).
smir ==> hype(300, 0.0001).
hype ==> fle(200, 0.0002).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% The prior probabilities of the hypotheses
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

prior(rcib) is 0.5.
prior(rcvp) is 0.5.
prior(smir) is 0.03.
prior(hype) is 0.01.
prior(fle)  is 0.005.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% The posterior probabilities represent what is currently known.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

posterior(rcib) is 1.
posterior(rcvp) is 1.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Call the prgram by: posterior(fle)?
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
