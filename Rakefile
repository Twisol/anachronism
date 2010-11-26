require 'rspec/core/rake_task'

RSpec::Core::RakeTask.new(:spec)
task :default => :spec

namespace :ragel do
  desc 'View a Ragel state machine visualization'
  task :graph, :file do |t, args|
    name = 'ext/anachronism/' + args['file']
    exec "ragel #{name}.rl -V -p | dot -Tpng > #{name}.png && eog #{name}.png && rm #{name}.png"
  end

  desc 'Build a Ragel state machine'
  task :build, :file do |t, args|
    name = 'ext/anachronism/' + args['file']
    exec "ragel -C -G2 #{name}.rl"    
  end
end

namespace :ext do
  desc 'Build the Ruby extension'
  task :build do
    exec "cd ext/anachronism/ && make"
  end
end
