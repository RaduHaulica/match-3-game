# Match 3 game

A basic match 3 puzzle game

![Match 3 cover image](https://github.com/RaduHaulica/match-3-game/blob/4f7af8388be19e01e3ffd687020d186d06467874/match%203%202022/media/match3%20cover.png)

Click gems and move them to create patterns of three or more of the same color in a aline to destroy them and rack in points.

If you move a gem and don't destroy anything, the move is reverted. If you mange to destroy gems there's a small time interval before new gems start dropping where you can chain another move to destroy more gems for bonus points.

Bonus points are also awarded for gems destroyed in addition to the minimum required of three and for random combo chains that result from gems falling into place.

Grey gems are wildcards and the red bomb destroys all adjacent gems when moved.

At each step, the game checks whether there is a valid move possible, and if none are found, it generates a new game board configuration.

Used observer pattern to play sounds and handle scoring, particle system to decorate gem destruction.

![Match 3 gameplay](https://github.com/RaduHaulica/match-3-game/blob/4f7af8388be19e01e3ffd687020d186d06467874/match%203%202022/media/match%203%20main.gif)