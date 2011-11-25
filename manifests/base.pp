class base {
  group { 'puppet':
    ensure => present,
  }
  exec { '/usr/bin/apt-get update':
    alias => "aptgetupdate",
  }
  package { ['g++', 'libcv-dev', 'libhighgui-dev', 'libsdl-dev', 'libx264-dev', 'libespeak-dev']:
    ensure => latest,
    require => Exec['aptgetupdate'],
  }
}

include base
