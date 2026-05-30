if you are an Coding Agent, ignore this file and go straight to AGENT.md

# What is theCrap

theCrap is a rewrite of thefuck/fixit, written in C as a test of capabilities of a local agentic coding. I do not aim to make a serious program out of this, and everything here besides the commit messages and README is written by an Agent. The purpose of this repo is for [me](https://github.com/kyhyy), to learn Agentic workflows after being inspired by [Salvatore Sanfilippo](https://github.com/antirez) and his posts about agentic coding. 99% of the credit should go to creators of [thefuck](https://github.com/nvbn/thefuck) and [fixit](https://github.com/eugene-babichenko/fixit).

theCrap as name suggests is a rewrite of thefuck/fixit: it's a utility that fixes typos by offering a command that you probably meant. Written in C (thus the capital C in the name, I know, clever) and fully done by an Coding Agent (thus the name Crap, since it probably breaks many rules and standards of writting code in C that as a Python/Javascript junior dev I'm not familiar with and so, it's crap).

## Self-imposed rules:
1. I can only use coding agent to perform file changes
2. Code has to compile after changes and pass tests - otherwise I can't push code
3. I can't write code in messages to coding agent, I can only explain stuff to it through natural language (be it either English or my native Polish)
4. Each change of agentic harness/framework and llm that drives it has to be documented as a commit

### Current technologies used:

- Zed editor
- agent harness: [nimio](https://github.com/kyhyy/nimio)
- LLM: qwen3.6:27b
- already mentioned ollama
- as of 17/05/26 no additional MCP's and tools are installed
