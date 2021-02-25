# Multi-Shot Solving Approach

This directory includes a generalized multi-shot approach developed to solve the [Aircraft Routing and Maintenance Planning](https://graz.pure.elsevier.com/en/publications/an-asp-multi-shot-encoding-for-the-aircraft-routing-and-maintenan) problem.

The problem incrementally grows at each iteration. 
We increase the problem size until we found a solution and then:

We set a time limit at each iteration without a new solution (parametrized by the **isecond** constant). 
If we found a new solution during an iteration, we reset the timer and continue to look for a solution.
We save the best solution found and go to the next iteration at the end of the timer, except if we have spent **iloop** iteration in a row without finding a new solution.

## Example calls

    clingo instance.lp example.lp multishot-py.lp
