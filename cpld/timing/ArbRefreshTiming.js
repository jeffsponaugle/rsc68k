{
    signal: [
     {node: '.........C.D..EF',period:2},
     {name: 'clk32',              wave:'101010101010101010101010101010101010101010101',node:'..A',period:2},
     {name: 'clk16',              wave:'010101010101010101010101',node: '.B..G.H',period:4,phase:-0.9},
     {name: 'cpu0 s',             wave:'X2...4...4...3...3...3...3...3...3...4...4...3...3...3...5.......3...3...3...4...4...3...3...3...0', data: ['S0', 'S1', 'S2', 'S3','S4','S5','S6','S7','S0', 'S1', 'S2', 'S3','S4','WS','S5','S6','S7','S0','S1','S2','S3','S4','S5','S6','S7','S0'],phase:0},
     {                            wave:'0..................7......0..............8..............0..7......0...............................', data:['DTACK Sample ^','REFRESH','DTACK Sample ^']},
     {name: 'cpu0_as',            wave:'1..............6...................1...........6...........................1............',data:['Memory Read','Memory Write','']},
     {name: 'cpu0_rw',            wave:'x........1.....................................0................................1.......'},
     {name: 'cpu0_uds/lds',       wave:'1..............0...................1..................0....................1............'},
     {name: 'cpu0_dram_cs',       wave:'1...............6...................1...........6...........................1...........',node:'....................................I', data:['CPU Memory Requested','CPU Memory Requested']},
     {name: 'cpu0_dram_dtack',    wave:'1......................0..............1........................0..............1.'},
     {name: 'cpu0_req_ck(clk32l)',wave:'1..................0..................1............0..........................1.'},
     {name: 'cpu0_gnt_ck(clk32h)',wave:'1....................6...............1.......................6...............1..',
                                  node:'.....................K...............J', data:['CPU Memory Granted','CPU Memory Granted']},
     {name: 'ref_req',            wave:'1.......................8................................1......................', data:['Refresh Requested']},
     {name: 'ref_req_ck(clk32l)', wave:'1..........................0...............................1....................'}, 
     {name: 'ref_gnt_ck(clk32h)', wave:'1........................................8...............1......................', data:['Refresh Granted']}, 
     {name: 'gnt_active',         wave:'1.....................0...............1...0...............1...0...............1.'},
     {name: 'dram_RASx',          wave:'1......................9.............1.........9........1......9.............1..', data:['RAS0,1,2,3','RAS0,1,2,3','RAS0,1,2,3'],
                                  node:'.......................L.......................................................'},
     {name: 'dram_AB',            wave:'1.......................0............1..........................0............1.....'},
     
     {name: 'dram_CASx',          wave:'1..........................3.........1.....3............1..........3.........1.....', data:['CAS0,2','CAS0,2','CAS0,2']},
     {name: 'dram_WR',            wave:'1............................................................0...................1................'},
     {name: 'dram_databus',       wave:'x.........................x...7......x...............7.....................x.......................',data:['Valid Data frm RAM','Valid Data frm CPU'],
      							  node:'..............................M..'},
     {name: 'refresh_complete',   wave:'1......................................................8..1........................................', data:['C']},
      
    ],
     
    config: {skin:'narrow'},
    
    edge: ['A~B 7 ns delay','C+D 31ns','E+F 15ns','G+H 62.5ns','I~J rst','K~>L triggers on next clk32 up','L-|-M 60ns'],
     foot:{
      text:'RSC68K Memory Access - With Arbitration',
      tock:-5,
       every:2
    },
   }