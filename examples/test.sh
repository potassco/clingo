#!/bin/bash
function check() {
    f="$1"
    shift
    cat <<EOF
${f}
\$ ${@}
EOF
    while :
    do
        (
            cd "$f"
            eval "${@}"
        )
        echo -n "looks alright (y/n)? [y] "
        read a
        clear
        [[ $a == "y" || $a == "" ]] && break
    done
}
cd "$(dirname "$0")"
clingo=$(realpath ../build/release/clingo)
export python=python
export PYTHONPATH=$(realpath ../build/release/python)

check clingo/addclause "${clingo}" addclause-py.lp 0
check clingo/addclause "${clingo}" addclause-lua.lp 0

check clingo/blocksworld cat control_01.lua \| "${clingo}" lua.lp blocks_01.lp world.lp lua_interpret.lp 0

check clingo/commit "${clingo}" commit-lua.lp encoding.lp
check clingo/commit "${clingo}" commit-py.lp encoding.lp

check clingo/consequences "${clingo}" brave-py.lp example.lp
check clingo/consequences "${clingo}" brave-lua.lp example.lp

check clingo/context "${clingo}" context-py.lp encoding.lp --outf=3 0
check clingo/context "${clingo}" context-lua.lp encoding.lp --outf=3 0

check clingo/controller-async "${python}" controller.py

function controller() {
    (sleep 1; "$python" <<q
import signal
signal.signal(signal.SIGINT, signal.SIG_IGN)
import client
q
) &
    pid=$!
    "$python" server.py
    kill $pid
}
check clingo/controller-processes controller

check clingo/controller-threads "$python" controller.py

check clingo/cover "${clingo}" control-py.lp
check clingo/cover "${clingo}" control-lua.lp

check clingo/domains "${clingo}" domains-py.lp instance.lp
check clingo/domains "${clingo}" domains-lua.lp instance.lp

check clingo/enum-assumption "${clingo}" example-py.lp
check clingo/enum-assumption "${clingo}" example-lua.lp

check clingo/expansion "${python}" ./main.py --verbose --option solver.forget_on_step 1 \
    GraphColouring/encodings/encoding.lp \
    GraphColouring/instances/0004-graph_colouring-125-0.lp

check clingo/expansion "${python}" main.py --verbose --option solver.forget_on_step 1 --maxobj 40 \
    PartnerUnits/encodings/encoding.lp \
    PartnerUnits/instances/180-partner_units_polynomial-47-0.lp

check clingo/external "${clingo}" external.lp external-py.lp
check clingo/external "${clingo}" external.lp external-lua.lp

check clingo/iclingo "${clingo}" incmode-int.lp example.lp
check clingo/iclingo "${clingo}" incmode-lua.lp example.lp
check clingo/iclingo "${clingo}" incmode-py.lp example.lp

check clingo/include "${clingo}" encoding-py.lp
check clingo/include "${clingo}" encoding-lua.lp

check clingo/incqueens "${clingo}" incqueens.lp incqueens-py.lp -c 'calls="list((1,1),(3,5),(8,9))"'
check clingo/incqueens "${clingo}" incqueens.lp incqueens-lua.lp -c 'calls="list((1,1),(3,5),(8,9))"'

check clingo/itersolve "${clingo}" itersolve-py.lp program.lp 0 -q
check clingo/itersolve "${clingo}" itersolve-lua.lp program.lp 0 -q

check clingo/load "${clingo}" load-py.lp
check clingo/load "${clingo}" load-lua.lp

check clingo/onmodel "${clingo}" onmodel-py.lp
check clingo/onmodel "${clingo}" onmodel-lua.lp

check clingo/planning "${clingo}" planning-lua.lp encoding.lp instances/coins01.lp
check clingo/planning "${clingo}" planning-lua.lp encoding.lp instances/comm02.lp

check clingo/pydoc "${python}" pydoc-lib.py

check clingo/robots "${python}" visualize.py

check clingo/setconf "${clingo}" setconf-py.lp
check clingo/setconf "${clingo}" setconf-lua.lp

check clingo/solitaire "${python}" visualize.py solitaire.lp instance.lp

check clingo/solve-async "${clingo}" solve-async-py.lp program.lp

check clingo/stats "${clingo}" example.lp stats-lua.lp
check clingo/stats "${clingo}" example.lp stats-py.lp

check clingo/unblock "${python}" visualize.py unblock.lp inst1.lp

check gringo/acyc "${clingo}" --mode=gringo encoding.lp instance.lp \| "${clingo}" --mode=clasp 0

check gringo/gbie "${clingo}" --mode=gringo gbie1.lp instances/sat_01.lp \| "${clingo}" --mode=clasp 0
check gringo/gbie "${clingo}" --mode=gringo gbie2.lp instances/sat_01.lp \| "${clingo}" --mode=clasp 0

check gringo/project "${clingo}" --mode=gringo example.lp \| "${clingo}" --mode=clasp --project 0

check gringo/queens "${clingo}" --mode=gringo queens1.lp \| "${clingo}" --mode=clasp 0
check gringo/queens "${clingo}" --mode=gringo queens2.lp \| "${clingo}" --mode=clasp 0

check gringo/rec-cond "${clingo}" --mode=gringo encoding.lp instance.lp \| "${clingo}" --mode=clasp 0

check gringo/subset "${clingo}" --mode=gringo example.lp \| "${clingo}" --mode=clasp --heuristic=domain --enum-mode=domRec 0

check gringo/toh "${clingo}" --mode=gringo -c imax=16 tohE.lp tohI.lp \| "${clingo}" --mode=clasp 0
