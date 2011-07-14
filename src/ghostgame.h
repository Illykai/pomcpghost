#ifndef GHOSTGAME_H
#define GHOSTGAME_H

#include "simulator.h"
#include "grid.h"
#include <list>

struct WALL
{
  COORD Position;
  int Width;
  int Height;
};

class GHOSTGAME_STATE : public STATE
{
public:
  
  COORD PlayerLoc;
  COORD SidekickLoc;
  int ActivePlayer;
  int TargetGhost;
  std::vector<COORD> GhostLocs;
  bool IsTerminalState() const;
};

class GHOSTGAME : public SIMULATOR
{
public:
  
  GHOSTGAME(const int width, const int height);
  
  virtual STATE* Copy(const STATE& state) const;
  virtual void Validate(const STATE& state) const;
  virtual STATE* CreateStartState() const;
  virtual void FreeState(STATE* state) const;
  virtual bool Step(STATE& state, int action, 
                    int& observation, double& reward) const;
  
  void GenerateLegal(const STATE& state, const HISTORY& history,
                     std::vector<int>& legal, const STATUS& status) const;
  virtual bool LocalMove(STATE& state, const HISTORY& history,
                         int stepObs, const STATUS& status) const;
  
  virtual void DisplayBeliefs(const BELIEF_STATE& beliefState, 
                              std::ostream& ostr) const;
  virtual void DisplayState(const STATE& state, std::ostream& ostr) const;
  virtual void DisplayObservation(const STATE& state, int observation, std::ostream& ostr) const;
  virtual void DisplayAction(int action, std::ostream& ostr) const;
  
private:
  bool IsValidLocation(const COORD& loc) const;
  void MoveIfValid(COORD& loc, const COORD& move) const;
  void MoveGhostIfValid(COORD& loc, const COORD& move, const GHOSTGAME_STATE& state) const;
  bool IsTerminalState(const GHOSTGAME_STATE& state) const;
  
  int Width, Height;
  std::vector<WALL> Walls;
  // for now we're going to hard-code in the number of ghosts
  static const int NUM_GHOSTS = 4; 
  
  mutable MEMORY_POOL<GHOSTGAME_STATE> MemoryPool;
};

#endif // GHOSTGAME_H
