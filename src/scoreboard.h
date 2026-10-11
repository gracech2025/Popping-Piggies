// Makes sure this file is only included once
#pragma once

// Create the scoreboard once
void createScoreboard();

// Update the HOME and GUEST scores
void updateScores(int homeScore, int guestScore);

// Update the bottom message
void updateMessage(const char* message);