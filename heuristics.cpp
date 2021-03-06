#include "heuristics.hpp"
#include "player.hpp"

namespace checkers {

int nodesSeen = 0;

constexpr int Heuristics::COEFFROWS[8];
constexpr int Heuristics::COEFFCOLS[8];
constexpr unsigned int Heuristics::CENTER[6];
constexpr unsigned int Heuristics::MAINDIAG[8];

// Top level minmax
// Special treatment for not returning a value but a gamestate
GameState Heuristics::topMinmax(Node root, const Deadline &pDue) {
   if(root.children.empty()) {
      return root.gameState;
   } else {
       int maxVal = -Heuristics::INFINITY;
       int currVal;
       GameState bestGS = root.children[0].gameState;
       int alpha = -Heuristics::INFINITY, beta = Heuristics::INFINITY;

       for(Node n : root.children) {
           if(pDue.getSeconds() - pDue.now().getSeconds() < 0.1) {
               std::cerr << "TIME !" << std::endl;
               return bestGS;
           }
           currVal = Heuristics::minmax(n, Heuristics::DEPTH - 1, false, pDue, alpha, beta);
           if(currVal > maxVal) {
               maxVal = currVal;
               bestGS = n.gameState;
               if(alpha < maxVal) {
                   alpha = maxVal;
               }
           }
       }

       std::cerr << "best val " << maxVal << std::endl;

       return bestGS;
   } 
}

int Heuristics::minmax(Node r, int depth, bool color, const Deadline &pDue,
        int alpha, int beta) {

    if(depth == 0) {


        ++nodesSeen;
        return Heuristics::evaluate(r.gameState, Player::color); 
    } else {
        
        Node root(r.gameState);
        root.mkTree(1, color);

        if(color) { // Main player : maximizing
            int maxVal = -Heuristics::INFINITY;
            int currVal;

            for(Node n : root.children) {

                currVal = Heuristics::minmax(n, depth - 1, !color, pDue, alpha, beta);
                if(currVal > maxVal) {
                    maxVal = currVal;

                    // Alpha cut ?
                    if(alpha < maxVal) {
                        alpha = maxVal;
                        if(alpha >= beta) {
                            return maxVal;
                        }
                    }
                }
            }

            return maxVal;
        } else {
            int minVal = Heuristics::INFINITY;
            int currVal;

            for(Node n : root.children) {

                currVal = Heuristics::minmax(n, depth - 1, !color, pDue, alpha, beta);
                if(currVal < minVal) {
                    minVal = currVal;

                    // Beta cut ?
                    if(beta > minVal) {
                        beta = minVal;
                        if(alpha >= beta) {
                            return minVal;
                        }
                    }
                }
            }

            return minVal;
        }
    }
}

int Heuristics::evaluate(GameState gs, uint8_t color) {

    uint8_t CELL_PLAYER, CELL_OPP;

    if(color == CELL_WHITE) {
        CELL_PLAYER = CELL_WHITE;
        CELL_OPP    = CELL_RED;
    } else {
        CELL_PLAYER = CELL_RED;
        CELL_OPP    = CELL_WHITE;
    }

    int pRow[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
    int pRowOpp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int kRow[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
    int kRowOpp[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int pCol[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
    int pColOpp[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    uint8_t currP;

    int evaluation   = 0;
    int pieces       = 0; int oppPieces       = 0;
    int kings        = 0; int oppKings        = 0;
    int centralKings = 0; int centralKingsOpp = 0;
    int diagKings    = 0; int diagKingsOpp    = 0;

    // Looping through board and counting
    for(unsigned int i = 0 ; i < 32 ; ++i) {
        currP = gs.at(GameState::cellToRow(i), GameState::cellToCol(i));
        if(currP & CELL_PLAYER) {
            pieces++;
            pCol[GameState::cellToCol(currP)] += 1;

            if(currP & CELL_KING) {
                kRow[GameState::cellToRow(currP)] += 1;
                kings++;
                // Is king centrally positionned ?
                for(unsigned int j = 0 ; j < 6 ; ++j) {
                    if(i == CENTER[j]) {
                        ++centralKings;
                        break;
                    }
                }
                // Is king on the main diagonal ?
                for(unsigned int j = 0 ; j < 8 ; ++j) {
                    if(i == MAINDIAG[j]) {
                        ++diagKings;
                        break;
                    }
                }
            } else { // Normal piece
                pRow[GameState::cellToRow(currP)] += 1;
            }
        } else if(currP & CELL_OPP) {
            oppPieces++;

            if(currP & CELL_KING) {
                oppKings++;
                 // Is king centrally positionned ?
                for(unsigned int j = 0 ; j < 6 ; ++j) {
                    if(i == CENTER[j]) {
                        ++centralKingsOpp;
                        break;
                    }
                }
                // Is king on the main diagonal ?
                for(unsigned int j = 0 ; j < 8 ; ++j) {
                    if(i == MAINDIAG[j]) {
                        ++diagKingsOpp;
                        break;
                    }
                }
            } else { // Normal piece
                pRowOpp[GameState::cellToRow(currP)] += 1;
            }
        }
    }

    // Infinite score (or -Infinity) if it's a winning or losing game state
    if(pieces == 0) {
        return -Heuristics::INFINITY;
    }
    if(oppPieces == 0) {
        return Heuristics::INFINITY;
    }

    // Early / mid game
    if((pieces - oppPieces) < 4 || oppPieces > 4) {
        // Combining row coeffs
        for(int i = 0 ; i < 8 ; i++) {
            if(CELL_PLAYER == CELL_RED) {
                evaluation += pRow[i] * Heuristics::COEFFROWS[i];
                evaluation -= pRowOpp[i] * Heuristics::COEFFROWS[7-i];

                // The closer to prom line (ie: 7-i small), the better. 
                // (The further for opponenent)
                evaluation -= pRow[i] * (7 - i); 
                evaluation += pRowOpp[i] * i;
            } else {
                evaluation += pRow[i] * Heuristics::COEFFROWS[7-i];
                evaluation -= pRowOpp[i] * Heuristics::COEFFROWS[i];
                
                // The closer to prom line (ie: i small), the better. 
                // (The further for opponenent)
                evaluation -= pRow[i] * i;
                evaluation += pRowOpp[i] * (7 - i);
            }
        }

        if(CELL_PLAYER == CELL_RED) {
            // Back pieces
            evaluation += Heuristics::BACKPIECE * pRow[0];
            evaluation -= Heuristics::BACKPIECE * pRowOpp[7];
        } else {
            // Back pieces
            evaluation += Heuristics::BACKPIECE * pRow[7];
            evaluation -= Heuristics::BACKPIECE * pRowOpp[0];
        }

        // Kings position
        evaluation += Heuristics::CENTRALKING * centralKings;
        evaluation -= Heuristics::CENTRALKING * centralKingsOpp;
        evaluation += Heuristics::CENTRALKING * diagKings;
        evaluation -= Heuristics::CENTRALKING * diagKingsOpp;

        // Columns coeffs
        for(int i = 0 ; i < 8 ; i++) {
            evaluation += pCol[i] * Heuristics::COEFFCOLS[i];
            evaluation -= pColOpp[i] * Heuristics::COEFFCOLS[i];
        }
        // Columns coeffs
        for(int i = 0 ; i < 8 ; i++) {
            evaluation += pRow[i] * Heuristics::COEFFCOLS[i];
            evaluation -= pRowOpp[i] * Heuristics::COEFFCOLS[i];
            evaluation += kRow[i] * Heuristics::COEFFCOLS[i];
            evaluation -= kRowOpp[i] * Heuristics::COEFFCOLS[i];
        }
    }

    evaluation += 
        pieces * Heuristics::PIECE - oppPieces * Heuristics::OPPPIECE;
        //kings * Heuristics::KING - oppKings * Heuristics::OPPKING; 

    return evaluation;
}

}
