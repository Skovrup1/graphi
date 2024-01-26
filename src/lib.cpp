class Foo {
  private:
    bool _cond;

  public:
    Foo(bool cond) { _cond = cond; }
    bool GetCond() { return _cond; }
};
