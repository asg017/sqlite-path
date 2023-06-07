require "version"

module SqlitePath
  class Error < StandardError; end
  def self.path_loadable_path
    File.expand_path('../path0', __FILE__)
  end
  def self.load(db)
    db.load_extension(self.path_loadable_path)
  end
end
