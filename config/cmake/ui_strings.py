import argparse
import csv

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, required=True)
    parser.add_argument('--output-defs', type=str, required=True)
    parser.add_argument('--output-text', type=str, required=True)
    parser.add_argument('--unicode-hex', action='store_true', required=False)
    args = parser.parse_args()

    with open(args.input, 'r') as f_in, open(args.output_defs, 'w') as f_od, open(args.output_text, 'w') as f_ot:
        reader = csv.reader(f_in)
        for row in reader:
            key, value = row
            if args.unicode_hex:
                text = chr(int(value, 16))
                f_od.write(f'#define {key} "\\u{value}"\n')
                f_ot.write(f'{text}\n')
            else:
                text = value.strip().replace('\\n', '')
                f_od.write(f'#define {key} "{value}"\n')
                f_ot.write(f'{text}\n')

if __name__ == '__main__':
    main()
