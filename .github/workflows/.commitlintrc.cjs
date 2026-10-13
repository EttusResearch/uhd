// Parse UHD's area-prefixed subjects and apply repository message limits.
module.exports = {
  parserPreset: {
    parserOpts: {
      headerPattern: /^(\S(?:.*\S)?): (\S(?:.*\S)?)$/,
      headerCorrespondence: ['scope', 'subject'],
    },
  },
  plugins: [
    {
      rules: {
        'header-ideal-length': (parsed, when, value) => {
          const isValid = parsed.header.length <= value;
          return [
            when === 'never' ? !isValid : isValid,
            `header should be ${value} characters or fewer`,
          ];
        },
        'no-fixup-commit': (parsed) => [
          !/^fixup!\s/i.test(parsed.header)
            && !/:\s*fixup!\s/i.test(parsed.header),
          'fixup commits are not allowed',
        ],
      },
    },
  ],
  defaultIgnores: false,
  rules: {
    'scope-empty': [2, 'never'],
    'subject-empty': [2, 'never'],
    'header-ideal-length': [1, 'always', 50],
    'header-max-length': [2, 'always', 72],
    'body-empty': [2, 'never'],
    'body-leading-blank': [2, 'always'],
    'body-max-line-length': [2, 'always', 72],
    'no-fixup-commit': [2, 'always'],
  },
};