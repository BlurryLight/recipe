import json
import time
from tencentcloud.common import credential
from tencentcloud.tmt.v20180321 import tmt_client, models
from tqdm import tqdm
import os
from itertools import islice
import re


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


def santinize_result(key: str, value: str):
    x = value
    if not ('（' in key):
        x = re.sub("\(.*?\)", "", x)

    if not ('：' in key):
        x = x.split("：")[0]

    return x


last_request_time = 0
secret_id = os.environ.get("tencent_secret_id")
secret_key = os.environ.get("tencent_secret_key")

if (len(secret_id) == 0 or len(secret_key) == 0):
    print("env error")
    os.sys.exit(0)

# 初始化腾讯云 API 密钥
cred = credential.Credential(secret_id, secret_key)
# 初始化翻译客户端
client = tmt_client.TmtClient(cred, "ap-chengdu")


def translate(texts, source_lang, target_lang, secret_id, secret_key):
    global last_request_time
    # 计算当前时间和上次请求时间的时间差
    time_diff = time.time() - last_request_time
    # 如果时间差小于0.2秒，则等待0.2秒后再发起请求
    if time_diff < 0.2:
        time.sleep(0.2 - time_diff)
    try:

        # 配置请求参数
        request = models.TextTranslateBatchRequest()
        request.Source = source_lang
        request.Target = target_lang
        #  request.SourceText = texts
        request.SourceTextList = texts
        request.ProjectId = 0
        # 发送请求并获取翻译结果
        response = client.TextTranslateBatch(request)
        translations = response.TargetTextList
        last_request_time = time.time()
        return translations

    except Exception as e:
        print(e)
        return None


with open('game.json', "r", encoding="utf-8") as f:
    # 你的待翻译文件名改为‘game.json’
    script = json.load(f)

# 原始文档中有一些key不应该被翻译
filtered_script = {k: v for k, v in script.items() if string_has_cjk(k)}

num = len(filtered_script.items())

pbar = tqdm(total=num, desc="翻译进度")

batch_size = 128
batches = [chunk for chunk in chunks(filtered_script, batch_size)]
for batch in batches:
    batch_text = []
    keys = []
    for key, value in batch.items():
        keys.append(key)
        batch_text.append(value)

    translated_text = translate(batch_text, "ja", "zh", secret_id, secret_key)
    for i in range(len(keys)):
        key = keys[i]
        value = translated_text[i]
        script[key] = santinize_result(key, value)
    with open("game_zh.json", "w", encoding="utf-8") as f:
        json.dump(script, f, ensure_ascii=False, indent=4)
    pbar.update(len(batch_text))


# 将翻译后的对话脚本保存到一个新的文件中

with open("game_zh.json", "w", encoding="utf-8") as f:
    json.dump(script, f, ensure_ascii=False, indent=4)

pbar.close()
