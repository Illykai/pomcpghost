#include "ghostgame.h"
#include "beliefstate.h"
#include "utils.h"
#include <math.h>
#include <iomanip>
#include <time.h>
#include <sstream>

using namespace std;
using namespace UTILS;

// NOTE: Currently ignoring the arguments just to get the damn thing started
GHOSTGAME::GHOSTGAME(const int width, const int height)
:   Width(4),
    Height(4)
{
  cout << "Making Ghost Game with width: " << Width << " height: " << Height << endl;
  NumActions = 5;
  NumObservations = 5;
  // don't know if this is actually correct...
  RewardRange = 1;
  Discount = 1;
  Walls.clear();
  WALL wall;
  COORD wallLoc(1,1);
  wall.Position = wallLoc;
  wall.Width = 2;
  wall.Height = 2;  
  Walls.push_back(wall);
}

STATE* GHOSTGAME::Copy(const STATE& state) const
{
  assert(state.IsAllocated());
  const GHOSTGAME_STATE& oldstate = safe_cast<const GHOSTGAME_STATE&>(state);
  GHOSTGAME_STATE* newstate = MemoryPool.Allocate();
  // this works because these states are structs - i have questions about the vector though...
  *newstate = oldstate;
  return newstate;
}

void GHOSTGAME::Validate(const STATE& state) const
{
  const GHOSTGAME_STATE& ghostState = safe_cast<const GHOSTGAME_STATE&>(state);
  bool valid = false;
  valid = IsValidLocation(ghostState.PlayerLoc);
  if(valid) 
  {
    valid = IsValidLocation(ghostState.SidekickLoc);
  }
  if(valid) 
  {
    vector<COORD>::const_iterator iter;
    for(iter = ghostState.GhostLocs.begin(); iter != ghostState.GhostLocs.end(); ++iter)
    {
      valid = IsValidLocation(*iter);
    }
  }
  if(!valid) 
  {
    DisplayState(ghostState, cout);
    assert(false);
  }
}

bool GHOSTGAME::IsValidLocation(const COORD& loc) const
{
  if (loc.X < 0 || loc.X >= Width) 
  {
    return false;
  }
  if (loc.Y < 0 || loc.Y >= Height) 
  {
    return false;
  }
  std::vector<WALL>::const_iterator iter;  
  for(iter = Walls.begin(); iter != Walls.end(); ++iter)
  {
    WALL wall = *iter;
    if (loc.X >= wall.Position.X && loc.X < wall.Position.X + wall.Width) 
    {
      if (loc.Y >= wall.Position.Y && loc.Y < wall.Position.Y + wall.Height) 
      {
        return false;
      }
    }
  }
  return true;
}
                              

void GHOSTGAME::MoveIfValid(COORD& loc, const COORD& move) const {
  COORD newLoc = loc + move;
  if(IsValidLocation(newLoc))
  {
    loc = newLoc;
  }
}

void GHOSTGAME::MoveGhostIfValid(COORD& loc, const COORD& move, const GHOSTGAME_STATE& state) const {
  COORD oldLoc = loc;
  // this is a bit silly, but it improves code reuse
  MoveIfValid(loc, move);
  if(loc == state.PlayerLoc || loc == state.SidekickLoc) 
  {
    // revert the move if it places them on top of one of the players
    loc = oldLoc;
  }
}


STATE* GHOSTGAME::CreateStartState() const
{
  GHOSTGAME_STATE* ghostGameState = MemoryPool.Allocate();
  // generate a state with randomized hidden state
  ghostGameState->PlayerLoc = COORD(0,0);
  ghostGameState->SidekickLoc = COORD(3,3);
  ghostGameState->GhostLocs.clear();
  ghostGameState->GhostLocs.push_back(COORD(3, 0));
  ghostGameState->GhostLocs.push_back(COORD(0, 3));
  ghostGameState->GhostLocs.push_back(COORD(2, 0));
  ghostGameState->GhostLocs.push_back(COORD(0, 2));
  ghostGameState->ActivePlayer = 0;
  // randomize the ghost that we're going for
  srand(time(NULL));
  ghostGameState->TargetGhost = rand()%4;
  return ghostGameState;
}

void GHOSTGAME::FreeState(STATE* state) const
{
  GHOSTGAME_STATE* ghostGameState = safe_cast<GHOSTGAME_STATE*>(state);
  MemoryPool.Free(ghostGameState);
}


bool GHOSTGAME::Step(STATE& state, int action,
                      int& observation, double& reward) const
{ 
  GHOSTGAME_STATE& ghostGameState = safe_cast<GHOSTGAME_STATE&>(state);
  observation = action;
  srand(time(NULL));
  COORD move;
  if (action < 4)
  {
    move = COORD::Compass[action];
  }
  else 
  {
    move = COORD(0,0);
  }
  COORD newLoc;
  if(ghostGameState.ActivePlayer == 0) 
  {
    MoveIfValid(ghostGameState.PlayerLoc, move);  
    if(ghostGameState.IsTerminalState())
    {
      //DisplayState(ghostGameState, cout);
      reward = 0.0;
      return true;
    }
  }
  else 
  {
    MoveIfValid(ghostGameState.SidekickLoc, move);
    if(ghostGameState.IsTerminalState())
    {
      reward = 0.0;
      return true;
    }
    // now move all the ghosts
    vector<COORD>::iterator iter;    
    for(iter = ghostGameState.GhostLocs.begin(); iter != ghostGameState.GhostLocs.end(); ++iter) 
    {
      int moveNum = rand() % 5;
      if (moveNum < 4) 
      {
        move = COORD::Compass[moveNum];
      }
      else 
      {
        move = COORD(0,0);
      }
      MoveGhostIfValid(*iter, move, ghostGameState);
    }
  }
  ghostGameState.ActivePlayer++;
  ghostGameState.ActivePlayer %= 2;
  reward = -1.0;
  return false;
}

bool GHOSTGAME::LocalMove(STATE& state, const HISTORY& history,
                           int stepObs, const STATUS& status) const
{
  GHOSTGAME_STATE& ghostGameState = safe_cast<GHOSTGAME_STATE&>(state);
  // permute the state
  srand(time(NULL));
  ghostGameState.TargetGhost = rand() % 2;
  return true;
}


void GHOSTGAME::GenerateLegal(const STATE& state, const HISTORY& history,
                               vector<int>& legal, const STATUS& status) const
{
  const GHOSTGAME_STATE& ghostGameState = safe_cast<const GHOSTGAME_STATE&>(state);
  // being a little bit smart here and eliminating actions that bash into walls
  COORD loc;
  if(ghostGameState.ActivePlayer == 0)
  {
    loc = ghostGameState.PlayerLoc;
  }
  else 
  {
    loc = ghostGameState.SidekickLoc;
  }

  for (int a = 0; a < NumActions-1; ++a) {
    COORD move = COORD::Compass[a];
    COORD locCopy = loc;
    MoveIfValid(locCopy, move);
    if(loc != locCopy)
    {
      legal.push_back(a);      
    }
  }
  // pass is always legal
  legal.push_back(4);
}

void GHOSTGAME::DisplayBeliefs(const BELIEF_STATE& beliefState,
                                ostream& ostr) const
{
  const GHOSTGAME_STATE* ghostGameState;
  int numGhosts = 0;
  if (beliefState.GetNumSamples() > 0)
  {
    ghostGameState = safe_cast<const GHOSTGAME_STATE*>(beliefState.GetSample(0));
    numGhosts = ghostGameState->GhostLocs.size();
  }
  
  vector<int> targetCounts(numGhosts, 0);
  for (int i = 0; i < beliefState.GetNumSamples(); i++)
  {
    ghostGameState = safe_cast<const GHOSTGAME_STATE*>(beliefState.GetSample(i));
    targetCounts[ghostGameState->TargetGhost]++;
  }
  for(unsigned int i = 0; i < targetCounts.size(); ++i) 
  {
      ostr << "Ghost " << i << ": " << (targetCounts[i] / beliefState.GetNumSamples()) << endl;
  }
}

void GHOSTGAME::DisplayState(const STATE& state, ostream& ostr) const
{
  // this isn't super elegant since it can end up cramming a lot of stuff into one cell, but it is what it is
  const GHOSTGAME_STATE& ghostGameState = safe_cast<const GHOSTGAME_STATE&>(state);
  ostr << endl << "  ";
  for (int x = 0; x < Width; x++)
    ostr << setw(1) << x << ' ';
  ostr << "  " << endl;
  for (int y = Height - 1; y >= 0; y--)
  {
    ostr << setw(1) << y << ' ';
    for (int x = 0; x < Width; x++)
    {
      COORD loc = COORD(x,y);
      std::stringstream   cell;
      bool isEmpty = true;
      if (!IsValidLocation(loc)) 
      {
        cell << "W";
        isEmpty = false;
      }
      else {
        if(loc == ghostGameState.PlayerLoc) 
        {
          cell << "P";
          isEmpty = false;
        }
        if(loc == ghostGameState.SidekickLoc) 
        {
          cell << "S";
          isEmpty = false;
        }
        for(unsigned int i = 0; i < ghostGameState.GhostLocs.size(); ++i) 
        {
          if(loc == ghostGameState.GhostLocs[i]) {
            cell << i;
            isEmpty = false;
          }
        }
      }
      if(isEmpty) 
      {
        cell << ".";
      }
      ostr << cell.str() << " ";
    }
    ostr << setw(1) << y << endl;
  }
  ostr << "  ";
  for (int x = 0; x < Height; x++)
    ostr << setw(1) << x << ' ';
  ostr << "  " << endl;
  ostr << "Active Player: " << ghostGameState.ActivePlayer << endl;
}

void GHOSTGAME::DisplayObservation(const STATE& state, int observation, ostream& ostr) const
{
  string move;
  if (observation < 4) 
  {
    move = COORD::CompassString[observation];
  } else 
  {
    move = "Pass";
  }
  
  ostr << "Observed " << move << endl;
}

void GHOSTGAME::DisplayAction(int action, ostream& ostr) const
{
  string move;
  if (action < 4) 
  {
    move = COORD::CompassString[action];
  } else 
  {
    move = "Pass";
  }
  ostr << "Action " << move << endl;
}


bool GHOSTGAME_STATE::IsTerminalState() const {
  // a state is terminal if the sidekick and player are both standing on one of the ghosts
  if (PlayerLoc == GhostLocs[TargetGhost] && SidekickLoc == GhostLocs[TargetGhost]) 
  {
    return true;
  }  
  return false;
//  if (PlayerLoc == SidekickLoc) 
//  {
//    vector<COORD>::const_iterator iter;    
//    for(iter = GhostLocs.begin(); iter != GhostLocs.end(); ++iter)
//    {
//      if(*iter == PlayerLoc)
//      {
//        return true;
//      }
//    }
//  }
  return false;
}