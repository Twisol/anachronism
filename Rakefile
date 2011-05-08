namespace :ragel do
  desc 'View a Ragel state machine visualization'
  task :graph, :file do |t, args|
    name = 'src/anachronism'
    exec "ragel #{name}.rl -V -p | dot -Tpng > #{name}.png && eog #{name}.png && rm #{name}.png"
  end
end
