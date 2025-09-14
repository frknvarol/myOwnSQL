RSpec.describe 'database' do
  def run_script(commands)
    raw_output = nil
    IO.popen("./build/mydb", "r+") do |pipe|
      commands.each do |command|
        pipe.puts command
      end

      pipe.close_write

      # Read entire output
      raw_output = pipe.gets(nil)
    end
    raw_output.split("\n")
  end

  it 'inserts and retrieves a row' do
    result = run_script([
      "create table tablo (1 int, 2 text)",
      "insert into tablo values (31, 'annen')",
      "select * from tablo",
      ".exit",
    ])
    expect(result).to match_array([
      "db > (stub) You entered SQL: create table tablo (1 int, 2 text)",
      "Table 'tablo' created with 2 columns.",
      "Executed.",
      "db > (stub) You entered SQL: insert into tablo values (31, 'annen')",
      "Executed.",
      "db > (stub) You entered SQL: select * from tablo",
      "(31, annen)",
      "Executed.",
      "db > ",
    ])
  end
end