namespace :ragel do
  desc 'View a Ragel state machine visualization'
  task :graph, :file do |t, args|
    name = args['file']
    exec "ragel #{name}.rl -V -p | dot -Tpng > #{name}.png && eog #{name}.png && rm #{name}.png"
  end

  desc 'Build a Ragel state machine'
  task :build, :file do |t, args|
    name = args['file']
    exec "ragel -C -G2 #{name}.rl"    
  end
end
