# AutoInf Core Engine Library

This library automatically 'plays' modern or Infocom-era text adventures for the [Z-machine](https://en.wikipedia.org/wiki/Z-machine): it incrementally builds the graph of the game's possible states and the actions required to reach them; building can be parallelised across multiple worker processes. Users can access information about each game state, which they may use to influence the order of processing (by, for example, focusing on the states that exhibit most progress through the game).

## Licence

The content of the AutoInf repository is free software; you can redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl-2.0.txt) as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

The content is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

## Quick Start

*   Building requires [SCons 2](http://scons.org/) and [buildtools](https://github.com/gcrossland/buildtools). Ensure that the contents of buildtools is available on PYTHONPATH e.g. `export PYTHONPATH=/path/to/buildtools` (or is in the SCons site_scons dir).
*   The library depends on [Core](https://github.com/gcrossland/Core), [Bitset](https://github.com/gcrossland/Bitset), [AutoFrotz](https://github.com/gcrossland/AutoFrotz), [Iterators](https://github.com/gcrossland/Iterators) and [IO](https://github.com/gcrossland/IO). Build these first.
*   From the working directory (or archive) root, run SCons to make a release build, specifying the compiler to use and where to find it e.g. `scons CONFIG=release TOOL_GCC=/usr/bin`.
    *   The library files are deployed to the library cache dir, which is (by default) under buildtools.
*   The library also comes with a testing demo, which is configured for playing a couple of games (a simple testing game and a port of [Adventure](https://en.wikipedia.org/wiki/Colossal_Cave_Adventure)). The demo lets you interactively control which of the game states to explore on from next and provides metrics that represent how much progress has been made at each game state. Input to the demo is in the form of lines of single-letter commands (making it easy to drive the demo by script).

### Testing Demo Walkthrough

*   Start playing the test game e.g. `./autoinf testgame cmd`. You may benefit from a display width of at least 120 columns of text.
*   At the top is a tree view of the game states reached so far (which shows only the starting state right now). Below that is the menu, with the command letter for each menu item underlined.
    ```
    (☐ 0) -> [&179D6EAA] {0, -42, 20, 0, 32766, 26988, 59754, 56708, 1} / unprocessed
    ☐ Hide Ḏead End Nodes                    Add W̱orker    
    ☐ Hide Aṉtiselected Nodes                Ṟemove Workers
    ☐ Hide Nodes Ḇeyond Depth...           ────────────────
    ☐ Com̱bine Similar Siblings               P̱rocess       
    ───────────────────────────────────      Coḻlapse      
      Select A̱ll                             Ṯerminate     
      Select U̱nprocesseds                  ────────────────
      C̱lear Selection                        Save̱ As...    
      I̱nvert Selection                       O̱pen...       
      Shrink Selection to Top-V̱alued...      Q̱uit          
    ───────────────────────────────────                    
      S̱how Output                                          
      H̱ide Output                                          
    ```
*   Give the demo a line of commands to _Select All_ game states and then _Process_ the only selected one. `a p`
    ```
    (☐ 0) -> [&179D6EAA] {0, -42, 20, 0, 0, -1, -1, 0, 1} / 14 children
    ├─(☐ 1) "examine red sphere" -> [&6A713E8F] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 2) "examine blue sphere" -> [&1652C399] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 3) "examine green sphere" -> [&EA7770B8] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 4) "examine wheel" -> [&D48D1068] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 5) "examine light" -> [&2BDCF639] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 6) "east" -> [&7652FDA6] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 7) "west" -> [&399CF629] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 8) "take red sphere" -> [&502D0FF5] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 9) "take blue sphere" -> [&70D07F71] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 10) "drop red sphere" -> [&BECDB8E3] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 11) "drop blue sphere" -> [&5D148675] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 12) "open red sphere" -> [&759EB30A] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 13) "open blue sphere" -> [&8A30CC88] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    └─(☐ 14) "turn wheel. pull wheel" -> [&75A278A0] {5, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    New words: red sphere, blue sphere, green sphere, wheel, light
    ```
*   The tree now shows the game states you can reach directly from the starting state, each with the corresponding action. Not all possible actions appear; the demo doesn't show those actions that don't make any material difference to the game.
*   Let's consider the game states reached after trying to examine things (labelled 1 to 5). We'll assume that none of these 'examine' actions made any material difference to the game, so the parent game state (labelled 0) and each of the game states afterwards should be the same. AutoInf is currently treating the game states as seperate because of spurious changes (e.g. in the buffer that the game uses while parsing incoming text). Educate AutoInf about this: select the parent game state and these five children and _Collapse_ them all together. `0 1-5 l`
    ```
    (☐ 0) -> [&AEADED2A] {0, -42, 20, 0, 0, -1, -1, 0, 1} / 7 children
    ├─(☐ 1) "take red sphere" -> [&9D44DFB0] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─{☐ 1} "take blue sphere" -> [&9D44DFB0] {0, -42, 33, 1, 1, -1, 0, 5000, 1}
    ├─(☐ 2) "drop red sphere" -> [&1E2671D6] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 3) "drop blue sphere" -> [&EF2231D8] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 4) "open red sphere" -> [&00870325] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─{☐ 4} "open blue sphere" -> [&00870325] {0, -42, 33, 1, 1, -1, 0, 5000, 1}
    └─(☐ 5) "turn wheel. pull wheel" -> [&A6EF66D1] {5, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ```
*   Notice how AutoInf now realises that some game states (labelled 1 and 4) can be reached by multiple actions. Set the tree view to _Combine Similar Siblings_, to group these together. `m`
    ```
    (☐ 0) -> [&AEADED2A] {0, -42, 20, 0, 0, -1, -1, 0, 1} / 7 children
    ├─(☐ 1) "take red sphere" "take blue sphere" -> [&9D44DFB0] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 2) "drop red sphere" -> [&1E2671D6] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 3) "drop blue sphere" -> [&EF2231D8] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ├─(☐ 4) "open red sphere" "open blue sphere" -> [&00870325] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    └─(☐ 5) "turn wheel. pull wheel" -> [&A6EF66D1] {5, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ```
*   Let's look more closely at the remaining game states. _Select All_ and _Show Output_ (and then _Clear Selection_). `a s c`
    ```
    (☐ 0) -> [&AEADED2A] {0, -42, 20, 0, 0, -1, -1, 0, 1} / 7 children
    ├─(☐ 1) "take red sphere" "take blue sphere" -> [&9D44DFB0] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    │    Main Room                                              0/1
    │   
    │   I only understood you as far as wanting to take the East Door.
    ├─(☐ 2) "drop red sphere" -> [&1E2671D6] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    │    Main Room                                              0/1
    │   
    │   I only understood you as far as wanting to drop the East Door.
    ├─(☐ 3) "drop blue sphere" -> [&EF2231D8] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    │    Main Room                                              0/1
    │   
    │   I only understood you as far as wanting to drop the West Door.
    ├─(☐ 4) "open red sphere" "open blue sphere" -> [&00870325] {0, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    │    Main Room                                              0/1
    │   
    │   I only understood you as far as wanting to open the East Door.
    └─(☐ 5) "turn wheel. pull wheel" -> [&A6EF66D1] {5, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
         Main Room                                              5/3
        
        With some effort, you twist the wheel anti-clockwise.
        You open the domed hatch, revealing a red sphere and a blue sphere.
        
        [Your score has just gone up by five points.]
    ```
*   The actions dealing with spheres (labelled 1 to 4) all do nothing (since there are no spheres present yet). Again, assume that these game states are not materially different to their parent and _Collapse_ them all together. `0 1-4 l`
    ```
    (☐ 0) -> [&8A7A60C7] {0, -42, 20, 0, 0, -1, -1, 0, 1} / 1 child
    └─(☐ 1) "turn wheel. pull wheel" -> [&9DEDB26C] {5, -42, 33, 1, 1, -1, 0, 5000, 1} / unprocessed
    ```
*   We have now narrowed the game down to one interesting initial action. Select it and then _Process_ it. `u p`
    ```
    (☐ 0) -> [&8A7A60C7] {0, -42, 20, 0, 0, -1, -1, 0, 1} / 1 child
    └─(☐ 1) "turn wheel. pull wheel" -> [&9DEDB26C] {5, -42, 33, 1, 1, -1, 0, 5000, 1} / 13 children
      ├─(☐ 2) "examine red sphere" "drop red sphere" "open red sphere" -> [&B3E7AC17] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / unprocessed
      ├─(☐ 3) "examine blue sphere" "drop blue sphere" "open blue sphere" -> [&7B7C4936] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / unprocessed
      ├─(☐ 4) "examine green sphere" -> [&B69704D8] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / unprocessed
      ├─(☐ 5) "examine wheel" "examine light" -> [&EC530EF8] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / unprocessed
      ├─(☐ 6) "east" -> [&4F02FEA3] {5, -42, 53, 1, 2, -1, 1, 6666, 1} / unprocessed
      ├─(☐ 7) "west" -> [&D64F5C48] {5, -42, 53, 1, 2, -1, 1, 6666, 1} / unprocessed
      ├─(☐ 8) "take red sphere" -> [&09D7A17F] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / unprocessed
      ├─(☐ 9) "take blue sphere" -> [&8824FED6] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / unprocessed
    New words: red sphere, blue sphere, green sphere, light
    ```
*   Let's start using the demo's metrics to focus in on the game states where we've made most progress. We'll use the first metric, labelled **a**, which is just the game score. Select the unprocessed game states and then _Shrink Selection to_ the top two highest-scoring game states, plus any more that are tied. `u v-a2+`
    ```
    Selected 8 (8 unprocessed) (from 8) nodes (threshold metric value 5)
    ```
*   Oh, dear. All of the selected game states have the same score, 5, so this hasn't helped narrow things down. Press forward by processing these game states and re-selecting the new high scorers. `p u v-a2+`
    <br>_(You're probably also running out of display space. We'll deal with this shortly.)_
    ```
    Selected 30 (30 unprocessed) (from 30) nodes (threshold metric value 5)
    ```
*   Still no differentiation. Try another round. `p u v-a2+`
    ```
    Selected 2 (2 unprocessed) (from 85) nodes (threshold metric value 10)
    ```
*   Progress! Exactly two game states have reached a score of 10. Hide from the tree view all game states except the two that we've selected (and their parents) and show their output. `n s`
    ```
    (☐ 0) -> [&8A7A60C7] {0, -42, 20, 0, 0, -1, -1, 0, 1} / 1 child
    └─(☐ 1) "turn wheel. pull wheel" -> [&9DEDB26C] {5, -42, 33, 1, 1, -1, 0, 5000, 1} / 13 children
      ├─(☐ 8) "take red sphere" -> [&09D7A17F] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / 13 children
      │ └─(☐ 26) "east" -> [&1D1FF884] {5, -42, 61, 0, 2, -2, 0, 5000, 1} / 10 children
      │   └─(☒ 75) "drop red sphere" -> [&E75AFF53] {10, -42, 74, 1, 3, -2, 1, 6000, 1} / unprocessed
      │        East Room                                              10/6
      │       
      │       As the sphere hits the ground, it is swallowed in a brief crimson
      │       flame and disappears.
      │       
      │       [Your score has just gone up by five points.]
      └─(☐ 9) "take blue sphere" -> [&8824FED6] {5, -42, 41, 1, 2, -1, 1, 6666, 1} / 13 children
        └─(☐ 36) "west" -> [&CE8D4323] {5, -42, 61, 0, 2, -2, 0, 5000, 1} / 10 children
          └─(☒ 110) "drop blue sphere" -> [&A94C9D75] {10, -42, 74, 1, 3, -2, 1, 6000, 1} / unprocessed
               West Room                                              10/6
              
              The sphere melts quickly, the water seeming to seep through the floor.
              
              [Your score has just gone up by five points.]
    ```
*   Process these and then re-select the top-scoring game states. `p u v-a1+`
    <br>_(And so on...)_
