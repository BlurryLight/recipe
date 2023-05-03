import json
import time
import requests
from tqdm import tqdm
import os
import sys
from itertools import islice
import re

import random
import json
import hashlib
import argparse


class BaiDuFanyi:
    def __init__(self, appKey, appSecret):
        self.url = 'https://fanyi-api.baidu.com/api/trans/vip/translate'
        self.appid = appKey
        self.secretKey = appSecret
        self.fromLang = 'jp'
        self.toLang = 'zh'
        self.salt = random.randint(32768, 65536)
        self.header = {'Content-Type': 'application/x-www-form-urlencoded'}

    def BdTrans(self, text):
        sign = self.appid + text + str(self.salt) + self.secretKey
        md = hashlib.md5()
        md.update(sign.encode(encoding='utf-8'))
        sign = md.hexdigest()
        data = {
            "appid": self.appid,
            "q": text,
            "from": self.fromLang,
            "to": self.toLang,
            "salt": self.salt,
            "sign": sign
        }
        response = requests.post(self.url, params=data, headers=self.header)  # 发送post请求
        text = response.json()  # 返回的为json格式用json接收数据
        return text


def is_cjk(character):
    """"
    Checks whether character is CJK.

        >>> is_cjk(u'\u33fe')
        True
        >>> is_cjk(u'\uFE5F')
        False

    :param character: The character that needs to be checked.
    :type character: char
    :return: bool
    """
    return any([start <= ord(character) <= end for start, end in
                [(4352, 4607), (11904, 42191), (43072, 43135), (44032, 55215),
                 (63744, 64255), (65072, 65103), (65381, 65500),
                 (131072, 196607)]
                ])


def string_has_cjk(s):
    return any(map(is_cjk, s))


def chunks(data, SIZE=1024):
    it = iter(data)
    for i in range(0, len(data), SIZE):
        yield {k: data[k] for k in islice(it, SIZE)}


last_request_time = 0
secret_id = os.environ.get("baidu_secret_id")
secret_key = os.environ.get("baidu_secret_key")

# Create ArgumentParser object
parser = argparse.ArgumentParser()

# Add arguments
parser.add_argument("-i", "--input", type=str, default="ManualTransFile.json", help="input JSON file name")
parser.add_argument("-o", "--output", type=str, default="TrsData.json", help="output JSON file name")

# Parse arguments
args = parser.parse_args()

# Access arguments
input_filename = args.input
output_filename = args.output

if (len(secret_id) == 0 or len(secret_key) == 0):
    print("env error")
    sys.exit(0)

BaiDuTranslator = BaiDuFanyi(secret_id, secret_key)

with open(input_filename, "r", encoding="utf-8") as f:
    script = json.load(f)

# 原始文档中有一些key不应该被翻译
filtered_script = {k: v for k, v in script.items() if string_has_cjk(k)}

num = len(filtered_script.items())

pbar = tqdm(total=num, desc="翻译进度")

batch_size = 32
batches = [chunk for chunk in chunks(filtered_script, batch_size)]
failed_batches = []


def translate(text):
    try:
        translated_json = BaiDuTranslator.BdTrans(text)
        if ('error_code' in translated_json):
            print("error code: {} error_msg {}".format(translated_json['error_code'], translated_json['error_msg']))
            return
        if 'trans_result' in translated_json:
            for item in translated_json['trans_result']:
                key = item['src']
                value = item['dst']
                script[key] = value
            with open(output_filename, "w", encoding="utf-8") as f:
                json.dump(script, f, ensure_ascii=False, indent=4)
        else:
            print(translated_json)
        return True
    except Exception as e:
        print("something wrong {}".format(e))
        return False


for batch in batches:
    batch_text = [value for value in batch.values()]
    if (not translate('\n'.join(batch_text))):
        failed_batches.append(batch_text)
    pbar.update(len(batch_text))
    time.sleep(0.2)

for batch_text in failed_batches:
    translated_json = BaiDuTranslator.BdTrans('\n'.join(batch))
    translate('\n'.join(batch_text))
    pbar.update(len(batch_text))
    time.sleep(0.2)

# 将翻译后的对话脚本保存到一个新的文件中

with open(output_filename, "w", encoding="utf-8") as f:
    json.dump(script, f, ensure_ascii=False, indent=4)

pbar.close()
